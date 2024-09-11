package test

import (
	"bufio"
	"bytes"
	"context"
	"fmt"
	"io"
	"os"
	"os/exec"
	"strings"
	"time"
)

func RunServer(ctx context.Context) (func(func(string, ...any)) error, error) {
	r, stdout := io.Pipe()

	var errR bytes.Buffer
	initR := io.TeeReader(r, &errR)

	cmd := exec.CommandContext(ctx, "../build/server")
	cmd.Stdout = stdout
	cmd.Stderr = stdout

	if err := cmd.Start(); err != nil {
		return nil, err
	}

	if err := waitServerReady(ctx, initR); err != nil {
		return nil, err
	}

	var stopped bool

	return func(logf func(string, ...any)) error {
		if stopped {
			return nil
		}
		stopped = true

		if err := cmd.Process.Signal(os.Interrupt); err != nil {
			logf("Failed to send SIGINT: %v", err)
		}

		killT := time.AfterFunc(5*time.Second, func() {
			logf("Server did not stop in time, sending SIGKILL")
			if err := cmd.Process.Kill(); err != nil {
				logf("Failed to send SIGKILL: %v", err)
			}
		})
		defer killT.Stop()

		err := cmd.Wait()
		if err != nil {
			return fmt.Errorf("failed to wait for server to stop: %v. Stdout+Stderr:\n%s", err, errR.String())
		}

		logf("Server stopped. Stdout+Stderr:\n%s", errR.String())
		return nil
	}, nil
}

func waitServerReady(ctx context.Context, stdout io.Reader) error {
	ctx, cancel := context.WithTimeout(ctx, 5*time.Second)
	defer cancel()

	ch := make(chan error, 1)

	go func() {
		defer io.ReadAll(stdout) // Drain the output
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
