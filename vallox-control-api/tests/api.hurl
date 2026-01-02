# Test Unauthorized Access
GET {{base_url}}/api/vallox?action=get&type=digit_vars
HTTP 404
[Asserts]
jsonpath "$.error" == "not authorized"

# Test Invalid Parameters
GET {{base_url}}/api/vallox?action=invalid&token={{token}}
HTTP 500
[Asserts]
jsonpath "$.error" == "invalid URL parameters"

# Test GET digit_vars
# Note: This might return 500 "device not responding" if offline, or 200 with data if online
GET {{base_url}}/api/vallox?action=get&type=digit_vars&token={{token}}
[Options]
very-verbose: true
HTTP/1.1 *
[Asserts]
header "Content-Type" contains "application/json"
# If it's a success, it should have id: 0
# If it's a failure (500), it should have an error message
# We use * for status code to handle both cases if we just want to see the output
# But for a proper test we could assert 200 if we expect it to be up

# Test SET FAN_SPEED
GET {{base_url}}/api/vallox?action=set&type=digit_vars&variable=FAN_SPEED&value=3&token={{token}}
[Options]
very-verbose: true
HTTP/1.1 *
[Asserts]
header "Content-Type" contains "application/json"
