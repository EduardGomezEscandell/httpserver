package test

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"os/exec"
	"time"
)

func RunServer(ctx context.Context) (func() error, error) {
	var stdout bytes.Buffer

	cmd := exec.CommandContext(ctx, "../build/server")
	cmd.Stdout = &stdout
	cmd.Stderr = &stdout
	if err := cmd.Start(); err != nil {
		return nil, err
	}
	time.Sleep(100 * time.Millisecond)

	return func() error {
		err := cmd.Wait()
		if errors.Is(err, context.Canceled) {
			return nil
		}
		if errors.Is(err, context.DeadlineExceeded) {
			return nil
		}
		if t := (&exec.ExitError{}); errors.As(err, &t) {
			return fmt.Errorf("%w, stderr: %s", t, stdout.String())
		}
		return err
	}, nil
}
