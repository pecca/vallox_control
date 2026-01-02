# Testing Vallox Control API

We use [Hurl](https://hurl.dev/) for testing the API endpoints. Hurl is a command-line tool that runs HTTP requests defined in a simple plain text format.

## Prerequisites

- [Hurl](https://hurl.dev/docs/installation.html) installed.
- The API server should be running (default port 3000 or as configured in `.env`).

## Running Tests

You can run the tests using the provided helper script:

```bash
cd vallox-control-api
./tests/run_tests.sh
```

Alternatively, run hurl directly:

```bash
hurl --test tests/api.hurl --variable token=your_token --variable base_url=http://localhost:3000
```

## Test Cases

The `tests/api.hurl` script covers:
1.  **Unauthorized Access**: Verifies that requests without a valid token return 404.
2.  **Invalid Parameters**: Verifies that requests with missing/wrong parameters return 500.
3.  **GET digit_vars**: Verifies that it returns a valid JSON (if the device is reachable).
4.  **SET FAN_SPEED**: Verifies the set operation (if the device is reachable).

> [!NOTE]
> Since the API interacts with a physical device over UDP, some tests might fail if the device is offline.
