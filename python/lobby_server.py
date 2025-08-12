#!/usr/bin/env python3
"""
Simple lobby server, used for testing and debugging Knights online
platform functionality.

This server maintains a list of lobbies and handles client connections
for creating, joining, listing, and leaving lobbies.

The client side of this can be found in src/online_platform/dummy_online_platform.cpp.
Note that this is *not* normally compiled into the Knights client -- it is
used only in special debugging builds (when ONLINE_PLATFORM_DUMMY is defined).

Binary Protocol:
- Message Type (1 byte): 
  0x01=LOGIN, 0x02=CREATE_LOBBY, 0x03=GET_LOBBY_LIST, 0x04=JOIN_LOBBY, 0x05=LEAVE_LOBBY,
  0x06=GET_LOBBY_INFO, 0x07=SET_LOBBY_INFO
- Message Length (4 bytes, little-endian): Length of data payload
- Data payload (variable length)

Responses:
- Status (1 byte): 0x00=SUCCESS, 0x01=ERROR
- Data Length (4 bytes, little-endian)
- Response data (variable length)

"""

import socket
import struct
import threading
import time
from typing import Dict, List, Set

# Protocol constants
MSG_LOGIN = 0x01
MSG_CREATE_LOBBY = 0x02
MSG_GET_LOBBY_LIST = 0x03
MSG_JOIN_LOBBY = 0x04
MSG_LEAVE_LOBBY = 0x05
MSG_GET_LOBBY_INFO = 0x06
MSG_SET_LOBBY_INFO = 0x07

STATUS_SUCCESS = 0x00
STATUS_ERROR = 0x01

class Lobby:
    def __init__(self, lobby_id: str, leader_user_id: str):
        self.lobby_id = lobby_id
        self.leader_user_id = leader_user_id
        self.members: Set[str] = {leader_user_id}
        self.created_time = time.time()
        self.status_code = 0  # 0 or above
    
    def add_member(self, user_id: str) -> bool:
        if user_id not in self.members:
            self.members.add(user_id)
            return True
        return False
    
    def remove_member(self, user_id: str) -> bool:
        if user_id in self.members:
            self.members.remove(user_id)
            
            # If the leader left and there are still members, select a new leader
            if user_id == self.leader_user_id and len(self.members) > 0:
                # Select the first remaining member as the new leader
                self.leader_user_id = next(iter(self.members))
            
            return True
        return False
    
    def is_empty(self) -> bool:
        return len(self.members) == 0
    
    def get_leader_id(self) -> str:
        return self.leader_user_id

class LobbyServer:
    def __init__(self, port: int = 12345):
        self.port = port
        self.lobbies: Dict[str, Lobby] = {}
        self.clients: Dict[socket.socket, str] = {}  # socket -> user_id
        self.user_lobbies: Dict[str, str] = {}  # user_id -> lobby_id
        self.next_lobby_id = 1
        self.lock = threading.Lock()
    
    def start(self):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind(('localhost', self.port))
        server_socket.listen(10)
        
        print(f"Lobby server listening on port {self.port}")
        
        try:
            while True:
                client_socket, address = server_socket.accept()
                print(f"Client connected from {address}")
                client_thread = threading.Thread(
                    target=self.handle_client, 
                    args=(client_socket,)
                )
                client_thread.daemon = True
                client_thread.start()
        except KeyboardInterrupt:
            print("Server shutting down...")
        finally:
            server_socket.close()
    
    def handle_client(self, client_socket: socket.socket):
        try:
            while True:
                # Read message type and length
                header = client_socket.recv(5)
                if len(header) != 5:
                    break
                
                msg_type = header[0]
                msg_length = struct.unpack('<I', header[1:5])[0]
                
                # Read payload
                payload = b''
                while len(payload) < msg_length:
                    chunk = client_socket.recv(msg_length - len(payload))
                    if not chunk:
                        break
                    payload += chunk
                
                if len(payload) != msg_length:
                    break
                
                # Handle message
                response = self.handle_message(client_socket, msg_type, payload)
                
                # Send response
                client_socket.send(response)
                
        except Exception as e:
            print(f"Error handling client: {e}")
        finally:
            self.cleanup_client(client_socket)
            client_socket.close()
    
    def handle_message(self, client_socket: socket.socket, msg_type: int, payload: bytes) -> bytes:
        try:
            if msg_type == MSG_LOGIN:
                return self.handle_login(client_socket, payload)
            elif msg_type == MSG_CREATE_LOBBY:
                return self.handle_create_lobby(client_socket, payload)
            elif msg_type == MSG_GET_LOBBY_LIST:
                return self.handle_get_lobby_list(client_socket, payload)
            elif msg_type == MSG_JOIN_LOBBY:
                return self.handle_join_lobby(client_socket, payload)
            elif msg_type == MSG_LEAVE_LOBBY:
                return self.handle_leave_lobby(client_socket, payload)
            elif msg_type == MSG_GET_LOBBY_INFO:
                return self.handle_get_lobby_info(client_socket, payload)
            elif msg_type == MSG_SET_LOBBY_INFO:
                return self.handle_set_lobby_info(client_socket, payload)
            else:
                return self.create_error_response(b"Unknown message type")
        except Exception as e:
            return self.create_error_response(f"Error: {e}".encode())
    
    def handle_login(self, client_socket: socket.socket, payload: bytes) -> bytes:
        user_id = payload.decode('utf-8')
        
        with self.lock:
            self.clients[client_socket] = user_id
        
        print(f"User {user_id} logged in")
        return self.create_success_response(b"OK")
    
    def handle_create_lobby(self, client_socket: socket.socket, payload: bytes) -> bytes:
        if client_socket not in self.clients:
            return self.create_error_response(b"Not logged in")
        
        user_id = self.clients[client_socket]
        
        with self.lock:
            lobby_id = str(self.next_lobby_id)
            self.next_lobby_id += 1
            
            lobby = Lobby(lobby_id, user_id)
            self.lobbies[lobby_id] = lobby
            self.user_lobbies[user_id] = lobby_id
        
        print(f"User {user_id} created lobby {lobby_id}")
        return self.create_success_response(lobby_id.encode())
    
    def handle_get_lobby_list(self, client_socket: socket.socket, payload: bytes) -> bytes:
        if client_socket not in self.clients:
            return self.create_error_response(b"Not logged in")
        
        with self.lock:
            lobby_ids = list(self.lobbies.keys())
        
        # Format: number of lobbies (4 bytes) + lobby IDs (null-terminated strings)
        response_data = struct.pack('<I', len(lobby_ids))
        for lobby_id in lobby_ids:
            response_data += lobby_id.encode() + b'\0'
        
        return self.create_success_response(response_data)
    
    def handle_join_lobby(self, client_socket: socket.socket, payload: bytes) -> bytes:
        if client_socket not in self.clients:
            return self.create_error_response(b"Not logged in")
        
        lobby_id = payload.decode('utf-8')
        user_id = self.clients[client_socket]
        
        with self.lock:
            if lobby_id not in self.lobbies:
                return self.create_error_response(b"Lobby not found")
            
            lobby = self.lobbies[lobby_id]
            lobby.add_member(user_id)
            self.user_lobbies[user_id] = lobby_id
        
        print(f"User {user_id} joined lobby {lobby_id}")
        
        # Return success with lobby leader ID
        leader_id = lobby.get_leader_id()
        return self.create_success_response(leader_id.encode())
    
    def handle_leave_lobby(self, client_socket: socket.socket, payload: bytes) -> bytes:
        if client_socket not in self.clients:
            return self.create_error_response(b"Not logged in")
        
        user_id = self.clients[client_socket]
        
        with self.lock:
            if user_id not in self.user_lobbies:
                return self.create_error_response(b"Not in any lobby")
            
            lobby_id = self.user_lobbies[user_id]
            lobby = self.lobbies[lobby_id]
            lobby.remove_member(user_id)
            del self.user_lobbies[user_id]
            
            # Clean up empty lobby
            if lobby.is_empty():
                del self.lobbies[lobby_id]
                print(f"Lobby {lobby_id} deleted (empty)")
        
        print(f"User {user_id} left lobby {lobby_id}")
        return self.create_success_response(b"OK")
    
    def handle_get_lobby_info(self, client_socket: socket.socket, payload: bytes) -> bytes:
        if client_socket not in self.clients:
            return self.create_error_response(b"Not logged in")
        
        lobby_id = payload.decode('utf-8')
        
        with self.lock:
            if lobby_id not in self.lobbies:
                return self.create_error_response(b"Lobby not found")
            
            lobby = self.lobbies[lobby_id]
            
            # Format: leader_id (null-terminated) + num_players (4 bytes) + status_code (4 bytes)
            response_data = lobby.get_leader_id().encode() + b'\0'
            response_data += struct.pack('<I', len(lobby.members))
            response_data += struct.pack('<i', lobby.status_code)
        
        return self.create_success_response(response_data)
    
    def handle_set_lobby_info(self, client_socket: socket.socket, payload: bytes) -> bytes:
        if client_socket not in self.clients:
            return self.create_error_response(b"Not logged in")
        
        user_id = self.clients[client_socket]
        
        with self.lock:
            if user_id not in self.user_lobbies:
                return self.create_error_response(b"Not in any lobby")
            
            lobby_id = self.user_lobbies[user_id]
            lobby = self.lobbies[lobby_id]
            
            # Only lobby leader can set lobby info
            if lobby.get_leader_id() != user_id:
                return self.create_error_response(b"Only lobby leader can set lobby info")
            
            # Payload is status_code (4 bytes)
            if len(payload) != 4:
                return self.create_error_response(b"Invalid payload size")
            
            status_code = struct.unpack('<i', payload[0:4])[0]
            
            # Update lobby status code (leader_id and num_players are managed server-side)
            lobby.status_code = status_code
        
        print(f"User {user_id} updated lobby {lobby_id} info: status_code={status_code}")
        return self.create_success_response(b"OK")
    
    def cleanup_client(self, client_socket: socket.socket):
        if client_socket in self.clients:
            user_id = self.clients[client_socket]
            
            with self.lock:
                # Remove user from any lobby they're in
                if user_id in self.user_lobbies:
                    lobby_id = self.user_lobbies[user_id]
                    lobby = self.lobbies[lobby_id]
                    lobby.remove_member(user_id)
                    del self.user_lobbies[user_id]
                    
                    # Clean up empty lobby
                    if lobby.is_empty():
                        del self.lobbies[lobby_id]
                        print(f"Lobby {lobby_id} deleted (empty)")
                
                del self.clients[client_socket]
            
            print(f"User {user_id} disconnected")
    
    def create_success_response(self, data: bytes) -> bytes:
        return struct.pack('<BI', STATUS_SUCCESS, len(data)) + data
    
    def create_error_response(self, data: bytes) -> bytes:
        return struct.pack('<BI', STATUS_ERROR, len(data)) + data

if __name__ == '__main__':
    server = LobbyServer()
    server.start()
