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
import tkinter as tk
from tkinter import ttk, messagebox
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
        self.status_key = ""  # LocalKey string
        self.param_key = None  # Optional parameter LocalKey string
        self.lobby_state = "JOINED"  # Lobby state: "JOINED" or "FAILED"
    
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
                print(f"{self.leader_user_id} is now leader of lobby {self.lobby_id}")
            
            return True
        return False
    
    def is_empty(self) -> bool:
        return len(self.members) == 0
    
    def get_leader_id(self) -> str:
        return self.leader_user_id
    
    def set_leader(self, new_leader_id: str) -> bool:
        if new_leader_id in self.members:
            self.leader_user_id = new_leader_id
            return True
        return False
    
    def set_state(self, new_state: str) -> bool:
        if new_state in ["JOINED", "FAILED"]:
            self.lobby_state = new_state
            return True
        return False

class LobbyServer:
    def __init__(self, port: int = 12345):
        self.port = port
        self.lobbies: Dict[str, Lobby] = {}
        self.clients: Dict[socket.socket, str] = {}  # socket -> user_id
        self.user_lobbies: Dict[str, str] = {}  # user_id -> lobby_id
        self.next_lobby_id = 1
        self.lock = threading.Lock()
        self.gui = None
    
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
            
            # Format: leader_id (null-terminated) + num_players (4 bytes) + 
            #         status_key (null-terminated) + has_param (1 byte) + 
            #         [optional param_key (null-terminated)] + lobby_state (null-terminated)
            response_data = lobby.get_leader_id().encode() + b'\0'
            response_data += struct.pack('<I', len(lobby.members))
            response_data += lobby.status_key.encode() + b'\0'
            
            # Add has_param flag and optional param_key
            if lobby.param_key is not None:
                response_data += struct.pack('<B', 1)  # has_param = true
                response_data += lobby.param_key.encode() + b'\0'
            else:
                response_data += struct.pack('<B', 0)  # has_param = false
            
            # Add lobby state
            response_data += lobby.lobby_state.encode() + b'\0'
        
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
            
            # Parse payload: status_key (null-terminated) + has_param (1 byte) + 
            #               [optional param_key (null-terminated)]
            try:
                pos = 0
                
                # Parse status key
                null_pos = payload.find(b'\0', pos)
                if null_pos == -1:
                    return self.create_error_response(b"Invalid payload format")
                
                status_key = payload[pos:null_pos].decode('utf-8')
                pos = null_pos + 1
                
                # Parse has_param flag
                if pos >= len(payload):
                    return self.create_error_response(b"Invalid payload format")
                
                has_param = payload[pos]
                pos += 1
                
                # Parse optional param_key
                param_key = None
                if has_param:
                    if pos >= len(payload):
                        return self.create_error_response(b"Invalid payload format")
                    
                    null_pos = payload.find(b'\0', pos)
                    if null_pos == -1:
                        return self.create_error_response(b"Invalid payload format")
                    
                    param_key = payload[pos:null_pos].decode('utf-8')
                
                # Update lobby status (leader_id and num_players are managed server-side)
                lobby.status_key = status_key
                lobby.param_key = param_key
                
            except UnicodeDecodeError:
                return self.create_error_response(b"Invalid UTF-8 encoding")
        
        print(f"User {user_id} updated lobby {lobby_id} info: status_key='{status_key}', param_key='{param_key}'")
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
    
    def change_leader(self, lobby_id: str, new_leader_id: str) -> bool:
        with self.lock:
            if lobby_id in self.lobbies:
                lobby = self.lobbies[lobby_id]
                if lobby.set_leader(new_leader_id):
                    print(f"Leader of lobby {lobby_id} changed to {new_leader_id}")
                    return True
        return False
    
    def set_lobby_state(self, lobby_id: str, new_state: str) -> bool:
        with self.lock:
            if lobby_id in self.lobbies:
                lobby = self.lobbies[lobby_id]
                if lobby.set_state(new_state):
                    print(f"Lobby {lobby_id} state changed to {new_state}")
                    return True
        return False

class LobbyServerGUI:
    def __init__(self, server: LobbyServer):
        self.server = server
        self.root = tk.Tk()
        self.root.title("Lobby Server")
        self.root.geometry("800x600")
        
        # Create main frames
        self.players_frame = ttk.LabelFrame(self.root, text="Connected Players")
        self.players_frame.pack(fill="both", expand=True, padx=10, pady=5)
        
        self.lobbies_frame = ttk.LabelFrame(self.root, text="Lobbies")
        self.lobbies_frame.pack(fill="both", expand=True, padx=10, pady=5)
        
        # Players list
        self.players_tree = ttk.Treeview(self.players_frame, columns=("User ID",), show="tree headings")
        self.players_tree.heading("#0", text="")
        self.players_tree.heading("User ID", text="User ID")
        self.players_tree.pack(fill="both", expand=True, padx=5, pady=5)
        
        # Lobbies list
        self.lobbies_tree = ttk.Treeview(self.lobbies_frame, columns=("ID", "Leader", "Members", "Status"), show="tree headings")
        self.lobbies_tree.heading("#0", text="")
        self.lobbies_tree.heading("ID", text="Lobby ID")
        self.lobbies_tree.heading("Leader", text="Leader")
        self.lobbies_tree.heading("Members", text="Members")
        self.lobbies_tree.heading("Status", text="Status")
        self.lobbies_tree.pack(fill="both", expand=True, padx=5, pady=5)
        
        # Control frame for leader change
        self.control_frame = ttk.Frame(self.root)
        self.control_frame.pack(fill="x", padx=10, pady=5)
        
        ttk.Label(self.control_frame, text="Change Leader:").grid(row=0, column=0, padx=5)
        
        self.lobby_var = tk.StringVar()
        self.lobby_combo = ttk.Combobox(self.control_frame, textvariable=self.lobby_var, state="readonly")
        self.lobby_combo.grid(row=0, column=1, padx=5)
        
        self.new_leader_var = tk.StringVar()
        self.leader_combo = ttk.Combobox(self.control_frame, textvariable=self.new_leader_var, state="readonly")
        self.leader_combo.grid(row=0, column=2, padx=5)
        
        self.change_button = ttk.Button(self.control_frame, text="Change Leader", command=self.change_leader)
        self.change_button.grid(row=0, column=3, padx=5)
        
        # Control frame for lobby state
        self.state_frame = ttk.Frame(self.root)
        self.state_frame.pack(fill="x", padx=10, pady=5)
        
        ttk.Label(self.state_frame, text="Set Lobby State:").grid(row=0, column=0, padx=5)
        
        self.state_lobby_var = tk.StringVar()
        self.state_lobby_combo = ttk.Combobox(self.state_frame, textvariable=self.state_lobby_var, state="readonly")
        self.state_lobby_combo.grid(row=0, column=1, padx=5)
        
        self.joined_button = ttk.Button(self.state_frame, text="Set JOINED", command=lambda: self.set_lobby_state("JOINED"))
        self.joined_button.grid(row=0, column=2, padx=5)
        
        self.failed_button = ttk.Button(self.state_frame, text="Set FAILED", command=lambda: self.set_lobby_state("FAILED"))
        self.failed_button.grid(row=0, column=3, padx=5)
        
        # Bind lobby selection to update members
        self.lobby_combo.bind("<<ComboboxSelected>>", self.on_lobby_selected)
        
        # Start update loop
        self.update_display()
    
    def on_lobby_selected(self, event=None):
        lobby_id = self.lobby_var.get()
        if lobby_id:
            with self.server.lock:
                if lobby_id in self.server.lobbies:
                    lobby = self.server.lobbies[lobby_id]
                    members = list(lobby.members)
                    self.leader_combo.configure(values=members)
                    if members:
                        self.new_leader_var.set(members[0])
    
    def change_leader(self):
        lobby_id = self.lobby_var.get()
        new_leader = self.new_leader_var.get()
        
        if not lobby_id or not new_leader:
            messagebox.showwarning("Warning", "Please select both lobby and new leader")
            return
        
        if self.server.change_leader(lobby_id, new_leader):
            pass
        else:
            messagebox.showerror("Error", "Failed to change leader")
    
    def set_lobby_state(self, new_state: str):
        lobby_id = self.state_lobby_var.get()
        
        if not lobby_id:
            messagebox.showwarning("Warning", "Please select a lobby")
            return
        
        if self.server.set_lobby_state(lobby_id, new_state):
            messagebox.showinfo("Success", f"Lobby {lobby_id} state set to {new_state}")
        else:
            messagebox.showerror("Error", f"Failed to set lobby state to {new_state}")
    
    def update_display(self):
        # Clear current display
        for item in self.players_tree.get_children():
            self.players_tree.delete(item)
        
        for item in self.lobbies_tree.get_children():
            self.lobbies_tree.delete(item)
        
        with self.server.lock:
            # Update players list
            connected_users = set(self.server.clients.values())
            for user_id in connected_users:
                self.players_tree.insert("", "end", values=(user_id,))
            
            # Update lobbies list
            lobby_ids = []
            for lobby_id, lobby in self.server.lobbies.items():
                members_str = ", ".join(lobby.members)
                status = f"{lobby.status_key if lobby.status_key else 'No status'} ({lobby.lobby_state})"
                self.lobbies_tree.insert("", "end", values=(lobby_id, lobby.leader_user_id, members_str, status))
                lobby_ids.append(lobby_id)
            
            # Update combo boxes
            self.lobby_combo.configure(values=lobby_ids)
            self.state_lobby_combo.configure(values=lobby_ids)
        
        # Schedule next update
        self.root.after(1000, self.update_display)  # Update every second
    
    def start(self):
        self.root.mainloop()

if __name__ == '__main__':
    server = LobbyServer()
    
    # Start server in a separate thread
    server_thread = threading.Thread(target=server.start)
    server_thread.daemon = True
    server_thread.start()
    
    # Start GUI
    gui = LobbyServerGUI(server)
    server.gui = gui
    gui.start()
