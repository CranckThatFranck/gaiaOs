#!/usr/bin/env python3
"""
Gaia OS - Vertex AI Muse Wonder 1.0 Bridge
Translates Meta Muse Code CLI and OpenAI-compatible requests
to Google Cloud Vertex AI Responses API for project negocio-de-bolso-492422.
Ultra-lightweight (< 12 MB RSS), zero external dependencies.
"""

import sys
import os
import time
import json
import threading
import subprocess
import urllib.request
import urllib.error
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler

HOST = "127.0.0.1"
PORT = 18787
PROJECT_ID = "negocio-de-bolso-492422"
LOCATION = "global"
VERTEX_ENDPOINT = f"https://aiplatform.googleapis.com/v1/projects/{PROJECT_ID}/locations/{LOCATION}/publishers/meta/v1/responses"
MODEL_ID = "muse-wonder-1.0"


class TokenManager:
    def __init__(self):
        self.lock = threading.Lock()
        self.token = None
        self.expiry = 0

    def get_token(self):
        with self.lock:
            now = time.time()
            if self.token and now < self.expiry:
                return self.token
            try:
                # Retrieve token using gcloud
                if os.getuid() == 0:
                    cmd = ["su", "-", "francisco", "-c", "gcloud auth print-access-token"]
                else:
                    cmd = ["/usr/bin/gcloud", "auth", "print-access-token"]
                res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
                token = res.stdout.strip()
                if not token:
                    raise ValueError("Empty token received from gcloud")
                self.token = token
                self.expiry = now + 2400  # Cache for 40 minutes (tokens valid for 60 min)
                sys.stdout.write(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Refreshed GCP Bearer token for francisco\n")
                sys.stdout.flush()
                return self.token
            except Exception as e:
                sys.stderr.write(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Error retrieving GCP token: {e}\n")
                sys.stderr.flush()
                raise

    def invalidate(self):
        with self.lock:
            self.token = None
            self.expiry = 0


token_manager = TokenManager()


class BridgeRequestHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, format, *args):
        # Concise logging
        sys.stdout.write(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] {self.address_string()} - {format % args}\n")
        sys.stdout.flush()

    def send_cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With")

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_cors_headers()
        self.end_headers()

    def do_GET(self):
        path = self.path.split("?")[0]
        if path in ("/health", "/", ""):
            data = {
                "status": "healthy",
                "service": "gaia-muse-vertex-bridge",
                "project": PROJECT_ID,
                "model": MODEL_ID,
                "endpoint": VERTEX_ENDPOINT
            }
            body = json.dumps(data, indent=2).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(body)
            return

        if path in ("/v1/muse-code/models", "/muse-code/models", "/v1/models", "/models"):
            catalog = {
                "object": "list",
                "data": [
                    {
                        "id": MODEL_ID,
                        "object": "model",
                        "created": 1794334929,
                        "owned_by": "meta",
                        "permission": [],
                        "root": MODEL_ID,
                        "parent": None
                    },
                    {
                        "id": f"meta/{MODEL_ID}",
                        "object": "model",
                        "created": 1794334929,
                        "owned_by": "meta",
                        "permission": [],
                        "root": MODEL_ID,
                        "parent": None
                    }
                ]
            }
            body = json.dumps(catalog).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(body)
            return

        self.send_error(404, f"Path not found: {path}")

    def do_POST(self):
        path = self.path.split("?")[0]
        content_length = int(self.headers.get("Content-Length", 0))
        req_body = self.rfile.read(content_length) if content_length > 0 else b"{}"

        # 1. Direct Responses API (Meta Muse Code CLI)
        if path in ("/v1/responses", "/responses"):
            self.handle_responses_api(req_body)
            return

        # 2. OpenAI-compatible Chat Completions API (VS Code Continue, Cline, Codex, etc.)
        if path in ("/v1/chat/completions", "/chat/completions"):
            self.handle_chat_completions(req_body)
            return

        self.send_error(404, f"POST path not found: {path}")

    def handle_responses_api(self, req_body):
        token = token_manager.get_token()
        headers = {
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
            "Accept": self.headers.get("Accept", "application/json")
        }

        # Normalize model in payload if needed
        try:
            payload = json.loads(req_body.decode("utf-8"))
            if payload.get("model") in (MODEL_ID, f"meta/{MODEL_ID}"):
                payload["model"] = MODEL_ID
            forward_data = json.dumps(payload).encode("utf-8")
        except Exception:
            forward_data = req_body

        req = urllib.request.Request(VERTEX_ENDPOINT, data=forward_data, headers=headers, method="POST")

        try:
            with urllib.request.urlopen(req, timeout=120) as resp:
                status = resp.status
                content_type = resp.headers.get("Content-Type", "application/json")
                is_sse = "text/event-stream" in content_type

                self.send_response(status)
                self.send_header("Content-Type", content_type)
                self.send_cors_headers()

                if is_sse:
                    self.send_header("Cache-Control", "no-cache")
                    self.send_header("Connection", "keep-alive")
                    self.send_header("Transfer-Encoding", "chunked")
                    self.end_headers()

                    # Stream SSE in chunks
                    while True:
                        chunk = resp.read(1024)
                        if not chunk:
                            break
                        # HTTP/1.1 chunked encoding: length in hex + \r\n + chunk + \r\n
                        chunk_header = f"{len(chunk):X}\r\n".encode("utf-8")
                        self.wfile.write(chunk_header + chunk + b"\r\n")
                        self.wfile.flush()
                    # Terminating chunk
                    self.wfile.write(b"0\r\n\r\n")
                    self.wfile.flush()
                else:
                    body = resp.read()
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)

        except urllib.error.HTTPError as e:
            if e.code == 401:
                token_manager.invalidate()
            err_body = e.read()
            self.send_response(e.code)
            self.send_header("Content-Type", e.headers.get("Content-Type", "application/json"))
            self.send_header("Content-Length", str(len(err_body)))
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(err_body)
        except Exception as e:
            err_msg = json.dumps({"error": {"message": str(e), "type": "bridge_error"}}).encode("utf-8")
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(err_msg)))
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(err_msg)

    def handle_chat_completions(self, req_body):
        """Translates OpenAI /v1/chat/completions into Vertex AI Responses API and maps output back."""
        try:
            req_json = json.loads(req_body.decode("utf-8"))
        except Exception as e:
            self.send_error(400, f"Invalid JSON: {e}")
            return

        messages = req_json.get("messages", [])
        stream = bool(req_json.get("stream", False))

        system_instructions = []
        conversation_parts = []

        for msg in messages:
            role = msg.get("role", "user")
            content = msg.get("content", "")
            if isinstance(content, list):
                # Handle multimodal / parts if any
                text_parts = [p.get("text", "") for p in content if isinstance(p, dict) and p.get("type") == "text"]
                content = " ".join(text_parts)
            if role == "system":
                system_instructions.append(content)
            elif role == "user":
                conversation_parts.append(f"User: {content}")
            elif role == "assistant":
                conversation_parts.append(f"Assistant: {content}")

        # If only one user message, pass it cleanly without "User: " prefix
        if len(conversation_parts) == 1 and conversation_parts[0].startswith("User: "):
            prompt_input = conversation_parts[0][6:]
        else:
            prompt_input = "\n\n".join(conversation_parts)

        vertex_payload = {
            "model": MODEL_ID,
            "stream": stream,
            "input": prompt_input
        }
        if system_instructions:
            vertex_payload["instructions"] = "\n".join(system_instructions)

        token = token_manager.get_token()
        headers = {
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json"
        }

        req = urllib.request.Request(
            VERTEX_ENDPOINT,
            data=json.dumps(vertex_payload).encode("utf-8"),
            headers=headers,
            method="POST"
        )

        created_ts = int(time.time())
        chat_id = f"chatcmpl-muse-{created_ts}"

        try:
            with urllib.request.urlopen(req, timeout=120) as resp:
                if stream:
                    self.send_response(200)
                    self.send_header("Content-Type", "text/event-stream")
                    self.send_header("Cache-Control", "no-cache")
                    self.send_header("Connection", "keep-alive")
                    self.send_cors_headers()
                    self.end_headers()

                    # Stream line by line and parse SSE
                    buffer = ""
                    current_event = ""
                    for raw_line in resp:
                        line = raw_line.decode("utf-8", errors="replace")
                        if line.startswith("event: "):
                            current_event = line[7:].strip()
                        elif line.startswith("data: "):
                            data_str = line[6:].strip()
                            if current_event == "response.output_text.delta":
                                try:
                                    delta_data = json.loads(data_str)
                                    delta_text = delta_data.get("delta", "")
                                    if delta_text:
                                        chunk_obj = {
                                            "id": chat_id,
                                            "object": "chat.completion.chunk",
                                            "created": created_ts,
                                            "model": MODEL_ID,
                                            "choices": [
                                                {
                                                    "index": 0,
                                                    "delta": {"content": delta_text},
                                                    "finish_reason": None
                                                }
                                            ]
                                        }
                                        self.wfile.write(f"data: {json.dumps(chunk_obj)}\n\n".encode("utf-8"))
                                        self.wfile.flush()
                                except Exception:
                                    pass
                            elif current_event == "response.completed":
                                stop_obj = {
                                    "id": chat_id,
                                    "object": "chat.completion.chunk",
                                    "created": created_ts,
                                    "model": MODEL_ID,
                                    "choices": [
                                        {
                                            "index": 0,
                                            "delta": {},
                                            "finish_reason": "stop"
                                        }
                                    ]
                                }
                                self.wfile.write(f"data: {json.dumps(stop_obj)}\n\n".encode("utf-8"))
                                self.wfile.write(b"data: [DONE]\n\n")
                                self.wfile.flush()
                        elif line == "\n" or line == "\r\n":
                            current_event = ""
                else:
                    body_bytes = resp.read()
                    vertex_resp = json.loads(body_bytes.decode("utf-8"))
                    extracted_text = ""
                    for item in vertex_resp.get("output", []):
                        if item.get("type") == "message":
                            for part in item.get("content", []):
                                if part.get("type") == "output_text":
                                    extracted_text += part.get("text", "")

                    chat_resp = {
                        "id": chat_id,
                        "object": "chat.completion",
                        "created": created_ts,
                        "model": MODEL_ID,
                        "choices": [
                            {
                                "index": 0,
                                "message": {
                                    "role": "assistant",
                                    "content": extracted_text
                                },
                                "finish_reason": "stop"
                            }
                        ],
                        "usage": vertex_resp.get("usage", {})
                    }
                    resp_bytes = json.dumps(chat_resp).encode("utf-8")
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.send_header("Content-Length", str(len(resp_bytes)))
                    self.send_cors_headers()
                    self.end_headers()
                    self.wfile.write(resp_bytes)

        except urllib.error.HTTPError as e:
            if e.code == 401:
                token_manager.invalidate()
            err_body = e.read()
            self.send_response(e.code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(err_body)))
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(err_body)
        except Exception as e:
            err_msg = json.dumps({"error": {"message": str(e), "type": "bridge_error"}}).encode("utf-8")
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(err_msg)))
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(err_msg)


def main():
    server = ThreadingHTTPServer((HOST, PORT), BridgeRequestHandler)
    sys.stdout.write(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] Gaia Muse Vertex Bridge listening on http://{HOST}:{PORT}\n")
    sys.stdout.write(f"Target: {VERTEX_ENDPOINT}\n")
    sys.stdout.flush()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        sys.stdout.write("Shutting down bridge...\n")
        server.server_close()


if __name__ == "__main__":
    main()
