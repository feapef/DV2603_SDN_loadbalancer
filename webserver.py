import http.server

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def send_response(self, code, message=None):
        # Ensure using HTTP/1.1 instead of HTTP/1.0
        self.protocol_version = "HTTP/1.1"
        super().send_response(code, message)

    def end_headers(self):
        # Explicitly set the Connection: keep-alive header for HTTP/1.1
        self.send_header("Connection", "keep-alive")
        super().end_headers()

# Run the server on port 8080
http.server.test(HandlerClass=MyHTTPRequestHandler, port=8081)
