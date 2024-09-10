package test

import (
	"bufio"
	"bytes"
	"context"
	"errors"
	"fmt"
	"io"
	"os"
	"os/exec"
	"strings"
)

func RunServer(ctx context.Context) (func() error, error) {
	var stderr bytes.Buffer

	stdout, w := io.Pipe()
	r := io.TeeReader(stdout, os.Stderr)

	cmd := exec.CommandContext(ctx, "../build/server")
	cmd.Stdout = w
	cmd.Stderr = &stderr

	if err := cmd.Start(); err != nil {
		return nil, err
	}

	if err := waitServerReady(ctx, r); err != nil {
		return nil, err
	}

	return func() error {
		err := cmd.Wait()
		if errors.Is(err, context.Canceled) {
			return nil
		}
		if errors.Is(err, context.DeadlineExceeded) {
			return nil
		}
		if t := (&exec.ExitError{}); errors.As(err, &t) {
			return fmt.Errorf("%w, stderr: %s", t, stderr.String())
		}
		return err
	}, nil
}

func waitServerReady(ctx context.Context, stdout io.Reader) error {
	ch := make(chan error, 1)

	go func() {
		defer close(ch)

		sc := bufio.NewScanner(stdout)
		for sc.Scan() {
			if strings.HasPrefix(sc.Text(), "Listening") {
				ch <- nil
				return
			}
		}

		if err := sc.Err(); err != nil {
			ch <- err
			return
		}

		ch <- fmt.Errorf("server never started")
	}()

	var err error
	select {
	case <-ctx.Done():
		err = ctx.Err()
	case err = <-ch:
	}

	if err != nil {
		return fmt.Errorf("server did not start: %w", err)
	}

	return nil
}
