package test_test

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"net/http"
	"testing"
	"time"

	"github.com/EduardGomezEscandell/httpserver/test"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestGet(t *testing.T) {
	t.Parallel()

	ctx := context.Background()

	testCases := map[string]struct {
		path        string
		want        int
		wantHeaders map[string]string
	}{
		"Good address responds with an OK": {path: "/home", want: http.StatusOK, wantHeaders: map[string]string{"Content-Type": "text/html"}},
		"Bad address responds with a 404":  {path: "/not-found", want: http.StatusNotFound},
		"Redirect returns 3xx":             {path: "/", want: http.StatusMovedPermanently, wantHeaders: map[string]string{"Location": "/home"}},
	}

	for name, tc := range testCases {
		t.Run(name, func(t *testing.T) {
			t.Parallel()

			ctx, cancel := context.WithCancel(ctx)
			defer cancel()

			port := test.ReservePort()
			addr := fmt.Sprintf("http://localhost:%d", port)

			close, err := test.RunServer(ctx, port)
			require.NoError(t, err, "Server should start without issues")
			defer close(t.Logf)

			req, err := http.NewRequestWithContext(ctx, http.MethodGet, addr+tc.path, nil)
			require.NoError(t, err, "Request should be created without issues")

			client := &http.Client{
				Timeout: 5 * time.Second,
				CheckRedirect: func(req *http.Request, via []*http.Request) error {
					// Prevent following redirects
					return http.ErrUseLastResponse
				},
			}

			resp, err := client.Do(req)
			require.NoError(t, err, "Request should be executed without issues")

			require.Equal(t, tc.want, resp.StatusCode, "Status code should be as expected")
			for k, v := range tc.wantHeaders {
				require.Equal(t, v, resp.Header.Get(k), "Header should be as expected")
			}

			require.NoError(t, close(t.Logf), "Server should stop without issues")
		})
	}
}

func TestPost(t *testing.T) {
	t.Parallel()
	ctx := context.Background()

	testCases := map[string]struct {
		path     string
		body     []byte
		want     int
		wantBody []byte
	}{
		"Good address with no body":     {path: "/parrot", body: []byte{}, wantBody: []byte{}},
		"Good address with text body":   {path: "/parrot", body: []byte("this is just some sample text")},
		"Good address with long body":   {path: "/parrot", body: bytes.Repeat([]byte("abcdefg"), 8*1024)},
		"Good address with binary body": {path: "/parrot", body: []byte{4, 3, 2, 1, 0, 1, 2, 3}},

		"Bad address":                       {path: "/not-found", want: http.StatusNotFound, wantBody: []byte("404 Not Found\n")},
		"Address where POST is not allowed": {path: "/home", body: []byte("this is just some sample text"), want: http.StatusMethodNotAllowed, wantBody: []byte("405 Method Not Allowed\n")},
	}

	for name, tc := range testCases {
		t.Run(name, func(t *testing.T) {
			t.Parallel()

			// Defaults
			if tc.want == 0 {
				tc.want = http.StatusOK
			}
			if tc.wantBody == nil {
				tc.wantBody = tc.body
			}

			ctx, cancel := context.WithCancel(ctx)
			defer cancel()

			port := test.ReservePort()
			addr := fmt.Sprintf("http://localhost:%d", port)

			close, err := test.RunServer(ctx, port)
			require.NoError(t, err, "Server should start without issues")
			defer close(t.Logf)

			req, err := http.NewRequestWithContext(ctx, http.MethodPost, addr+tc.path, bytes.NewReader(tc.body))
			require.NoError(t, err, "Request should be created without issues")

			resp, err := (&http.Client{Timeout: 5 * time.Second}).Do(req)
			require.NoError(t, err, "Request should be executed without issues")
			require.Equal(t, tc.want, resp.StatusCode, "Status code should be as expected")

			bod, err := io.ReadAll(resp.Body)
			require.NoError(t, err)
			require.Equal(t, tc.wantBody, bod, "Body should be as expected")

			require.NoError(t, close(t.Logf), "Server should stop without issues")
		})
	}
}

func TestBadMethod(t *testing.T) {
	t.Parallel()
	ctx := context.Background()

	testCases := map[string]struct {
		path   string
		method string
		want   int
	}{
		"PUT is not implemented":     {path: "/home", method: http.MethodPut, want: http.StatusMethodNotAllowed},
		"PATCH is not implemented":   {path: "/home", method: http.MethodPatch, want: http.StatusMethodNotAllowed},
		"DELETE is not implemented":  {path: "/home", method: http.MethodDelete, want: http.StatusMethodNotAllowed},
		"OPTIONS is not implemented": {path: "/home", method: http.MethodOptions, want: http.StatusMethodNotAllowed},
		"HEAD is not implemented":    {path: "/home", method: http.MethodHead, want: http.StatusMethodNotAllowed},
	}

	for name, tc := range testCases {
		t.Run(name, func(t *testing.T) {
			t.Parallel()

			ctx, cancel := context.WithCancel(ctx)
			defer cancel()

			port := test.ReservePort()
			addr := fmt.Sprintf("http://localhost:%d", port)

			close, err := test.RunServer(ctx, port)
			require.NoError(t, err, "Server should start without issues")
			defer close(t.Logf)

			req, err := http.NewRequestWithContext(ctx, tc.method, addr+tc.path, nil)
			require.NoError(t, err, "Request should be created without issues")

			resp, err := (&http.Client{Timeout: 5 * time.Second}).Do(req)
			require.NoError(t, err, "Request should be executed without issues")
			require.Equal(t, tc.want, resp.StatusCode, "Status code should be as expected")

			require.NoError(t, close(t.Logf), "Server should stop without issues")
		})
	}
}

func TestMultithreding(t *testing.T) {
	// This test is not parallel because it times the response time,
	// and it would be hard to make it deterministic with parallel tests

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	port := test.ReservePort()
	addr := fmt.Sprintf("http://localhost:%d", port)

	stop, err := test.RunServer(ctx, port)
	require.NoError(t, err, "Server should start without issues")
	defer stop(t.Logf)

	ch := make(chan error)
	const nmessages = 32

	started := time.Now()

	for i := range nmessages {
		go func() {
			req, err := http.NewRequestWithContext(ctx, http.MethodPost, addr+"/sleep", nil)
			if err != nil {
				ch <- fmt.Errorf("request %d should be created without issues: %v", i, err)
				return
			}

			resp, err := (&http.Client{Timeout: 10 * time.Second}).Do(req)
			if err != nil {
				ch <- fmt.Errorf("request %d should be executed without issues: %v", i, err)
				return
			}

			if resp.StatusCode != http.StatusOK {
				ch <- fmt.Errorf("request %d: status code should be OK: %d", i, resp.StatusCode)
				return
			}

			ch <- nil
		}()
	}

	for i := 0; i < nmessages; i++ {
		assert.NoErrorf(t, <-ch, "Response %d", i)
	}

	finished := time.Now()
	assert.WithinDuration(t, started, finished, 5*time.Second, "All requests should be done in parallel")

	require.NoError(t, stop(t.Logf), "Server should stop without issues")
}
