package test_test

import (
	"context"
	"net/http"
	"path"
	"testing"
	"time"

	"github.com/EduardGomezEscandell/httpserver/test"
	"github.com/stretchr/testify/require"
)

const URL = "localhost:8080"

func TestGet(t *testing.T) {
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
			ctx, cancel := context.WithTimeout(ctx, 5*time.Second)
			defer cancel()

			close, err := test.RunServer(ctx)
			require.NoError(t, err, "Server should start without issues")
			defer func() {
				if err := close(); err != nil {
					t.Logf("Error closing server: %v", err)
				}
			}()

			req, err := http.NewRequestWithContext(ctx, http.MethodGet, "http://"+path.Join(URL, tc.path), nil)
			require.NoError(t, err, "Request should be created without issues")

			client := &http.Client{
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
		})
	}
}

func TestNotGET(t *testing.T) {
	ctx := context.Background()

	testCases := map[string]struct {
		path   string
		method string
		want   int
	}{
		"Wrong address is more important than method": {path: "/not-found", method: http.MethodPost, want: http.StatusNotFound},
		"POST is not implemented":                     {path: "/home", method: http.MethodPost, want: http.StatusMethodNotAllowed},
		"PUT is not implemented":                      {path: "/home", method: http.MethodPut, want: http.StatusMethodNotAllowed},
		"PATCH is not implemented":                    {path: "/home", method: http.MethodPatch, want: http.StatusMethodNotAllowed},
		"DELETE is not implemented":                   {path: "/home", method: http.MethodDelete, want: http.StatusMethodNotAllowed},
		"OPTIONS is not implemented":                  {path: "/home", method: http.MethodOptions, want: http.StatusMethodNotAllowed},
		"HEAD is not implemented":                     {path: "/home", method: http.MethodHead, want: http.StatusMethodNotAllowed},
	}

	for name, tc := range testCases {
		t.Run(name, func(t *testing.T) {
			ctx, cancel := context.WithTimeout(ctx, 5*time.Second)
			defer cancel()

			close, err := test.RunServer(ctx)
			require.NoError(t, err, "Server should start without issues")
			defer func() {
				if err := close(); err != nil {
					t.Logf("Error closing server: %v", err)
				}
			}()

			req, err := http.NewRequestWithContext(ctx, tc.method, "http://"+path.Join(URL, tc.path), nil)
			require.NoError(t, err, "Request should be created without issues")

			client := &http.Client{
				CheckRedirect: func(req *http.Request, via []*http.Request) error {
					// Prevent following redirects
					return http.ErrUseLastResponse
				},
			}

			resp, err := client.Do(req)
			require.NoError(t, err, "Request should be executed without issues")
			require.Equal(t, tc.want, resp.StatusCode, "Status code should be as expected")
		})
	}
}
