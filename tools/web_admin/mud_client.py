"""
MUD Admin API Client - Unix socket connector for Bylins MUD Admin API
"""
import socket
import json


class MudAdminClient:
    """Client for communicating with MUD Admin API via Unix socket"""

    def __init__(self, socket_path='admin_api.sock', username=None, password=None):
        self.socket_path = socket_path
        self.username = username
        self.password = password
        self.authenticated = False
    
    def _recv_line(self, sock, max_size=1048576):
        """Read data until newline or max_size"""
        data = b''
        while len(data) < max_size:
            chunk = sock.recv(1)
            if not chunk:
                break
            data += chunk
            if chunk == b'\n':
                break
        return data

    def _send_command(self, command, **params):
        """Send command to Admin API and return response"""
        try:
            # Connect to Unix socket
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.settimeout(30.0)  # 30 seconds for complex operations
            sock.connect(self.socket_path)

            # Read greeting (until newline)
            greeting = self._recv_line(sock)

            # Authenticate if credentials provided
            if self.username and self.password:
                auth_cmd = {"command": "auth", "username": self.username, "password": self.password}
                sock.sendall((json.dumps(auth_cmd) + '\n').encode())
                auth_resp_data = self._recv_line(sock)
                auth_resp = json.loads(auth_resp_data.decode())

                if auth_resp.get('status') != 'ok':
                    raise ValueError(f"Authentication failed: {auth_resp.get('error', 'Unknown error')}")

            # Send actual command
            request = {"command": command}
            request.update(params)
            sock.sendall((json.dumps(request) + '\n').encode())

            # Receive response (read until newline)
            response_data = self._recv_line(sock)
            sock.close()

            if not response_data:
                return {"status": "error", "error": "No response from server"}

            return json.loads(response_data.decode())
            
        except socket.timeout:
            return {"status": "error", "error": "Connection timeout"}
        except ConnectionRefusedError:
            return {"status": "error", "error": "Cannot connect to MUD server. Is it running?"}
        except Exception as e:
            return {"status": "error", "error": f"Connection error: {str(e)}"}
    
    # Zone operations
    def list_zones(self):
        """Get list of all zones"""
        return self._send_command("list_zones")
    
    def get_zone(self, vnum):
        """Get zone details by vnum"""
        return self._send_command("get_zone", vnum=vnum)
    
    def update_zone(self, vnum, data):
        """Update zone"""
        return self._send_command("update_zone", vnum=vnum, data=data)
    
    # Mob operations
    def list_mobs(self, zone):
        """Get list of mobs in zone"""
        return self._send_command("list_mobs", zone=str(zone))
    
    def get_mob(self, vnum):
        """Get mob details by vnum"""
        return self._send_command("get_mob", vnum=vnum)
    
    def update_mob(self, vnum, data):
        """Update mob"""
        return self._send_command("update_mob", vnum=vnum, data=data)
    
    def create_mob(self, zone, data):
        """Create new mob in zone"""
        return self._send_command("create_mob", zone=zone, data=data)
    
    # Object operations
    def list_objects(self, zone):
        """Get list of objects in zone"""
        return self._send_command("list_objects", zone=str(zone))
    
    def get_object(self, vnum):
        """Get object details by vnum"""
        return self._send_command("get_object", vnum=vnum)
    
    def update_object(self, vnum, data):
        """Update object"""
        return self._send_command("update_object", vnum=vnum, data=data)
    
    def create_object(self, zone, data):
        """Create new object in zone"""
        return self._send_command("create_object", zone=zone, data=data)
    
    # Room operations
    def list_rooms(self, zone):
        """Get list of rooms in zone"""
        return self._send_command("list_rooms", zone=str(zone))
    
    def get_room(self, vnum):
        """Get room details by vnum"""
        return self._send_command("get_room", vnum=vnum)
    
    def update_room(self, vnum, data):
        """Update room"""
        return self._send_command("update_room", vnum=vnum, data=data)
    
    def create_room(self, zone, data):
        """Create new room in zone"""
        return self._send_command("create_room", zone=zone, data=data)
    
    # Trigger operations
    def list_triggers(self, zone):
        """Get list of triggers in zone"""
        return self._send_command("list_triggers", zone=str(zone))
    
    def get_trigger(self, vnum):
        """Get trigger details by vnum"""
        return self._send_command("get_trigger", vnum=vnum)
    
    def update_trigger(self, vnum, data):
        """Update trigger"""
        return self._send_command("update_trigger", vnum=vnum, data=data)
    
    def create_trigger(self, zone, data):
        """Create new trigger in zone"""
        return self._send_command("create_trigger", zone=zone, data=data)

    def get_stats(self):
        """Get server statistics (zones, mobs, objects, rooms, triggers counts)"""
        return self._send_command("get_stats")

    def get_players(self):
        """Get list of online players"""
        return self._send_command("get_players")
