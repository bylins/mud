#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Test script for Admin API CRUD operations."""

import socket
import json
import sys
import os

SOCKET_PATH = os.getenv("SOCKET_PATH", "admin_api.sock")

def send_command(sock, command, **kwargs):
    """Send JSON command and receive response."""
    request = {"command": command, **kwargs}
    message = json.dumps(request) + "\n"
    sock.sendall(message.encode('utf-8'))

    # Read response
    buffer = b''
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("Connection closed")
        buffer += chunk
        if b'\n' in buffer:
            line, buffer = buffer.split(b'\n', 1)
            return json.loads(line.decode('utf-8'))

def verify_yaml_file(entity_type, vnum, expected_fields):
    """Verify that YAML file was actually saved with expected values."""
    import yaml
    
    # Remove vnum from expected_fields - it's not in YAML content
    expected_fields = {k: v for k, v in expected_fields.items() if k != 'vnum'}
    
    # Map flat API fields to YAML structure
    yaml_expected = {}
    for key, value in expected_fields.items():
        if key == 'name':
            # For mobs/objects, 'name' maps to 'names.aliases'
            # For rooms/zones/triggers, 'name' stays in root
            if entity_type in ['mob', 'object']:
                yaml_expected.setdefault('names', {})['aliases'] = value
            else:
                yaml_expected['name'] = value
        elif key == 'aliases':
            yaml_expected.setdefault('names', {})['aliases'] = value
        elif key == 'short_desc':
            if entity_type == 'mob':
                yaml_expected.setdefault('descriptions', {})['short_desc'] = value
            else:
                yaml_expected['short_desc'] = value
        elif key == 'long_desc':
            if entity_type == 'mob':
                yaml_expected.setdefault('descriptions', {})['long_desc'] = value
            else:
                yaml_expected['long_desc'] = value
        elif key == 'level':
            if entity_type == 'mob':
                yaml_expected.setdefault('stats', {})['level'] = value
            else:
                yaml_expected['level'] = value
        else:
            yaml_expected[key] = value
    
    expected_fields = yaml_expected
    world_dir = os.environ.get('WORLD_DIR', os.path.expanduser('~/repos/world.yaml'))

    if entity_type == 'mob':
        yaml_path = f"{world_dir}/world/mobs/{vnum}.yaml"
    elif entity_type == 'object':
        yaml_path = f"{world_dir}/world/objects/{vnum}.yaml"
    elif entity_type == 'room':
        zone = vnum // 100
        rel_num = vnum % 100
        yaml_path = f"{world_dir}/world/zones/{zone}/rooms/{rel_num:02d}.yaml"
    else:
        return False, "Unknown entity type"

    if not os.path.exists(yaml_path):
        return False, f"YAML file not found: {yaml_path}"

    try:
        with open(yaml_path, 'r', encoding='koi8-r') as f:
            data = yaml.safe_load(f)

        def check_fields(expected, actual, prefix=""):
            """Recursively check that expected fields are in actual data."""
            for key, expected_val in expected.items():
                field_name = f"{prefix}.{key}" if prefix else key
                if key not in actual:
                    return False, f"Field '{field_name}' not in YAML"
                actual_val = actual[key]
                # If expected is dict, recurse
                if isinstance(expected_val, dict):
                    if not isinstance(actual_val, dict):
                        return False, f"Field '{field_name}' is not a dict in YAML"
                    result, msg = check_fields(expected_val, actual_val, field_name)
                    if not result:
                        return result, msg
                # Otherwise check value match
                elif actual_val != expected_val:
                    return False, f"Field '{field_name}': expected {expected_val}, got {actual_val}"
            return True, "OK"

        result, msg = check_fields(expected_fields, data)
        if not result:
            return False, msg

        return True, "YAML verified"
    except Exception as e:
        return False, f"Error reading YAML: {e}"

def test_get_mob(sock, vnum):
    """Test get_mob command."""
    print(f"\n=== Testing get_mob {vnum} ===")
    response = send_command(sock, "get_mob", vnum=vnum)

    if response.get("status") == "ok":
        mob = response.get("mob", {})
        print(f"✓ Mob {vnum}: {mob.get('names', {}).get('nominative', 'N/A')}")
        print(f"  Level: {mob.get('stats', {}).get('level', 'N/A')}")
        print(f"  Aliases: {mob.get('names', {}).get('aliases', 'N/A')}")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_update_mob(sock, vnum, data):
    """Test update_mob command."""
    print(f"\n=== Testing update_mob {vnum} ===")
    response = send_command(sock, "update_mob", vnum=vnum, data=data)

    if response.get("status") == "ok":
        print(f"✓ Mob {vnum} updated successfully")

        # Verify YAML file
        success, msg = verify_yaml_file('mob', vnum, data)
        if success:
            print(f"  ✓ YAML file verified: {msg}")
        else:
            print(f"  ✗ YAML verification failed: {msg}")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_get_object(sock, vnum):
    """Test get_object command."""
    print(f"\n=== Testing get_object {vnum} ===")
    response = send_command(sock, "get_object", vnum=vnum)
    print(f"DEBUG: Full response: {response}")

    if response.get("status") == "ok":
        obj = response.get("object", {})
        print(f"✓ Object {vnum}: {obj.get('short_desc', 'N/A')}")
        print(f"  Aliases: {obj.get('aliases', 'N/A')}")
        print(f"  Type: {obj.get('type', 'N/A')}")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_update_object(sock, vnum, data):
    """Test update_object command."""
    print(f"\n=== Testing update_object {vnum} ===")
    response = send_command(sock, "update_object", vnum=vnum, data=data)

    if response.get("status") == "ok":
        print(f"✓ Object {vnum} updated successfully")

        # Verify YAML file
        success, msg = verify_yaml_file('object', vnum, data)
        if success:
            print(f"  ✓ YAML file verified: {msg}")
        else:
            print(f"  ✗ YAML verification failed: {msg}")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_get_room(sock, vnum):
    """Test get_room command."""
    print(f"\n=== Testing get_room {vnum} ===")
    response = send_command(sock, "get_room", vnum=vnum)

    if response.get("status") == "ok":
        room = response.get("room", {})
        print(f"✓ Room {vnum}: {room.get('name', 'N/A')}")
        print(f"  Description: {room.get('description', 'N/A')[:50]}...")
        print(f"  Sector: {room.get('sector_type', 'N/A')}")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_update_room(sock, vnum, data):
    """Test update_room command."""
    print(f"\n=== Testing update_room {vnum} ===")
    response = send_command(sock, "update_room", vnum=vnum, data=data)

    if response.get("status") == "ok":
        print(f"✓ Room {vnum} updated successfully")

        # Verify YAML file
        success, msg = verify_yaml_file('room', vnum, data)
        if success:
            print(f"  ✓ YAML file verified: {msg}")
        else:
            print(f"  ✗ YAML verification failed: {msg}")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_list_mobs(sock, zone):
    """Test list_mobs command."""
    print(f"\n=== Testing list_mobs zone {zone} ===")
    response = send_command(sock, "list_mobs", zone=zone)

    if response.get("status") == "ok":
        mobs = response.get("mobs", [])
        print(f"✓ Found {len(mobs)} mobs")
        for mob in mobs[:5]:
            print(f"  - {mob['vnum']}: {mob['name']} (level {mob['level']})")
        if len(mobs) > 5:
            print(f"  ... and {len(mobs) - 5} more")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_list_objects(sock, zone):
    """Test list_objects command."""
    print(f"\n=== Testing list_objects zone {zone} ===")
    response = send_command(sock, "list_objects", zone=zone)

    if response.get("status") == "ok":
        objects = response.get("objects", [])
        print(f"✓ Found {len(objects)} objects")
        for obj in objects[:5]:
            print(f"  - {obj['vnum']}: {obj['short_desc']}")
        if len(objects) > 5:
            print(f"  ... and {len(objects) - 5} more")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_list_rooms(sock, zone):
    """Test list_rooms command."""
    print(f"\n=== Testing list_rooms zone {zone} ===")
    response = send_command(sock, "list_rooms", zone=zone)

    if response.get("status") == "ok":
        rooms = response.get("rooms", [])
        print(f"✓ Found {len(rooms)} rooms")
        for room in rooms[:5]:
            print(f"  - {room['vnum']}: {room['name']}")
        if len(rooms) > 5:
            print(f"  ... and {len(rooms) - 5} more")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_list_zones(sock):
    """Test list_zones command."""
    print(f"\n=== Testing list_zones ===")
    response = send_command(sock, "list_zones")

    if response.get("status") == "ok":
        zones = response.get("zones", [])
        print(f"✓ Found {len(zones)} zones")
        for zone in zones[:10]:
            print(f"  - Zone {zone['vnum']}: {zone.get('name', 'N/A')}")
        if len(zones) > 10:
            print(f"  ... and {len(zones) - 10} more")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_create_mob(sock, zone, data):
    """Test create_mob command."""
    print(f"\n=== Testing create_mob in zone {zone} ===")
    response = send_command(sock, "create_mob", zone=zone, data=data)

    if response.get("status") == "ok":
        vnum = response.get("vnum", "?")
        print(f"✓ Mob created with vnum {vnum}")
        if "olc_output" in response:
            print(f"  OLC output: {response['olc_output']}")

        # Verify YAML file
        if vnum != "?":
            success, msg = verify_yaml_file('mob', vnum, data)
            if success:
                print(f"  ✓ YAML file verified: {msg}")
            else:
                print(f"  ✗ YAML verification failed: {msg}")

        return vnum
    else:
        print(f"✗ Error: {response.get('error')}")
        return None

def test_delete_mob(sock, vnum):
    """Test delete_mob command."""
    print(f"\n=== Testing delete_mob {vnum} ===")
    response = send_command(sock, "delete_mob", vnum=vnum)

    if response.get("status") == "ok":
        print(f"✓ Mob {vnum} deleted successfully")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_create_object(sock, zone, data):
    """Test create_object command."""
    print(f"\n=== Testing create_object in zone {zone} ===")
    response = send_command(sock, "create_object", zone=zone, data=data)

    if response.get("status") == "ok":
        vnum = response.get("vnum", "?")
        print(f"✓ Object created with vnum {vnum}")
        if "olc_output" in response:
            print(f"  OLC output: {response['olc_output']}")

        # Verify YAML file
        if vnum != "?":
            success, msg = verify_yaml_file('object', vnum, data)
            if success:
                print(f"  ✓ YAML file verified: {msg}")
            else:
                print(f"  ✗ YAML verification failed: {msg}")

        return vnum
    else:
        print(f"✗ Error: {response.get('error')}")
        return None

def test_delete_object(sock, vnum):
    """Test delete_object command."""
    print(f"\n=== Testing delete_object {vnum} ===")
    response = send_command(sock, "delete_object", vnum=vnum)

    if response.get("status") == "ok":
        print(f"✓ Object {vnum} deleted successfully")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_create_room(sock, zone, data):
    """Test create_room command."""
    print(f"\n=== Testing create_room in zone {zone} ===")
    response = send_command(sock, "create_room", zone=zone, data=data)

    if response.get("status") == "ok":
        vnum = response.get("vnum", "?")
        print(f"✓ Room created with vnum {vnum}")
        if "olc_output" in response:
            print(f"  OLC output: {response['olc_output']}")

        # Verify YAML file
        if vnum != "?":
            success, msg = verify_yaml_file('room', vnum, data)
            if success:
                print(f"  ✓ YAML file verified: {msg}")
            else:
                print(f"  ✗ YAML verification failed: {msg}")

        return vnum
    else:
        print(f"✗ Error: {response.get('error')}")
        return None

def test_delete_room(sock, vnum):
    """Test delete_room command."""
    print(f"\n=== Testing delete_room {vnum} ===")
    response = send_command(sock, "delete_room", vnum=vnum)

    if response.get("status") == "ok":
        print(f"✓ Room {vnum} deleted successfully")
    else:
        print(f"✗ Error: {response.get('error')}")

    return response

def test_comprehensive_mob(sock, vnum):
    """Test ALL mob fields comprehensively."""
    print(f"\n=== Testing comprehensive mob update {vnum} ===")
    
    comprehensive_data = {
        "names": {
            "aliases": "comprehensive тест",
        },
        "descriptions": {
            "short_desc": "Comprehensive test mob stands here.",
            "long_desc": "A mob used for comprehensive field testing."
        },
        "stats": {
            "level": 25,
            "hitroll_penalty": 5,
            "armor": 10,
            "exp": 1000
        },
        "sex": 0,
        "position": 8,
        "default_position": 8
    }
    
    # Update with comprehensive data
    response = send_command(sock, "update_mob", vnum=vnum, data=comprehensive_data)
    
    if response.get("status") == "ok":
        print(f"✓ Comprehensive mob update successful")
        
        # Verify by getting mob back
        mob = test_get_mob(sock, vnum)
        if mob:
            print(f"  ✓ Verified: level={mob.get('level')}, sex={mob.get('sex')}")
    else:
        print(f"✗ Error: {response.get('error', 'Unknown error')}")

def test_comprehensive_object(sock, vnum):
    """Test ALL object fields comprehensively."""
    print(f"\n=== Testing comprehensive object update {vnum} ===")
    
    comprehensive_data = {
        "aliases": "comprehensive тестобъект",
        "short_desc": "comprehensive test object",
        "weight": 10,
        "cost": 500,
        "rent_on": 250,
        "rent_off": 125,
        "type": 1,  # LIGHT
        "material": 1,
        "level": 15,
        "current_durability": 90,
        "maximum_durability": 100
    }
    
    response = send_command(sock, "update_object", vnum=vnum, data=comprehensive_data)
    
    if response.get("status") == "ok":
        print(f"✓ Comprehensive object update successful")
        
        obj = test_get_object(sock, vnum)
        if obj:
            print(f"  ✓ Verified: weight={obj.get('weight')}, cost={obj.get('cost')}, material={obj.get('material')}")
    else:
        print(f"✗ Error: {response.get('error', 'Unknown error')}")

def test_comprehensive_room(sock, vnum):
    """Test ALL room fields comprehensively."""
    print(f"\n=== Testing comprehensive room update {vnum} ===")
    
    comprehensive_data = {
        "name": "Comprehensive Test Room",
        "description": "This room is used for comprehensive field testing.",
        "sector_type": 0,  # INSIDE
        "triggers": [100]  # Add trigger 100
    }
    
    response = send_command(sock, "update_room", vnum=vnum, data=comprehensive_data)
    
    if response.get("status") == "ok":
        print(f"✓ Comprehensive room update successful")
        
        room = test_get_room(sock, vnum)
        if room:
            triggers = room.get('triggers', [])
            print(f"  ✓ Verified: sector={room.get('sector_type')}, triggers={triggers}")
    else:
        print(f"✗ Error: {response.get('error', 'Unknown error')}")


def main():
    if len(sys.argv) < 2:
        print("Usage: test_admin_api.py <command> [args...]")
        print("Commands:")
        print("  get_mob <vnum>")
        print("  update_mob <vnum>")
        print("  list_mobs <zone>")
        print("  get_object <vnum>")
        print("  update_object <vnum>")
        print("  list_objects <zone>")
        print("  get_room <vnum>")
        print("  update_room <vnum>")
        print("  list_rooms <zone>")
        print("  list_zones")
        print("  create_delete_test")
        print("  full_crud_test")
        sys.exit(1)

    command = sys.argv[1]

    # Connect to Unix socket
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect(SOCKET_PATH)

        # Read welcome message (sent automatically on connect)
        buffer = b''
        while b'\n' not in buffer:
            chunk = sock.recv(4096)
            if not chunk:
                raise ConnectionError("Connection closed")
            buffer += chunk
        welcome_line, _ = buffer.split(b'\n', 1)
        welcome = json.loads(welcome_line.decode('utf-8'))
        print(f"Connected: {welcome}")

        # Authenticate (using environment variables)
        username = os.getenv("MUD_USERNAME", "TestUser")
        password = os.getenv("MUD_PASSWORD", "")

        if not password:
            print("Error: MUD_PASSWORD environment variable not set")
            print("Usage: MUD_USERNAME=YourName MUD_PASSWORD=YourPassword ./test_admin_api.py <command>")
            sys.exit(1)

        auth_response = send_command(sock, "auth", username=username, password=password)
        print(f"DEBUG: Auth response: {auth_response}")
        if auth_response.get("status") != "ok":
            print(f"✗ Authentication failed: {auth_response.get('error')}")
            sys.exit(1)
        print("✓ Authenticated")

        # Execute command
        if command == "get_mob":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 100
            test_get_mob(sock, vnum)

        elif command == "update_mob":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 100

            # Get current mob
            mob = test_get_mob(sock, vnum)
            if mob.get("status") != "ok":
                sys.exit(1)

            # Update with test data
            data = {
                "names": {
                    "nominative": "тестовый моб (обновлен)"
                },
                "stats": {
                    "level": 42
                }
            }
            test_update_mob(sock, vnum, data)

            # Verify update
            test_get_mob(sock, vnum)

        elif command == "get_object":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 100
            test_get_object(sock, vnum)

        elif command == "update_object":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 100

            # Get current object
            obj = test_get_object(sock, vnum)
            if obj.get("status") != "ok":
                sys.exit(1)

            # Update with test data
            data = {
                "short_desc": "тестовый объект (обновлен)",
                "aliases": "тест объект обновлен"
            }
            test_update_object(sock, vnum, data)

            # Verify update
            test_get_object(sock, vnum)

        elif command == "get_room":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 3000
            test_get_room(sock, vnum)

        elif command == "update_room":
            vnum = int(sys.argv[2]) if len(sys.argv) > 2 else 3000

            # Get current room
            room = test_get_room(sock, vnum)
            if room.get("status") != "ok":
                sys.exit(1)

            # Update with test data
            data = {
                "name": "Тестовая комната (обновлена)",
                "description": "Это тестовое описание комнаты."
            }
            test_update_room(sock, vnum, data)

            # Verify update
            test_get_room(sock, vnum)

        elif command == "list_mobs":
            zone = sys.argv[2] if len(sys.argv) > 2 else "1"
            test_list_mobs(sock, zone)

        elif command == "list_objects":
            zone = sys.argv[2] if len(sys.argv) > 2 else "1"
            test_list_objects(sock, zone)

        elif command == "list_rooms":
            zone = sys.argv[2] if len(sys.argv) > 2 else "1"
            test_list_rooms(sock, zone)

        elif command == "list_zones":
            test_list_zones(sock)

        elif command == "create_delete_test":
            # Test creating and deleting entities
            print("\n" + "="*60)
            print("TESTING CREATE/DELETE MOB")
            print("="*60)

            mob_data = {
                "name": "тестмоб",
                "short_desc": "тестовый моб для создания",
                "level": 10
            }
            mob_vnum = test_create_mob(sock, 1, mob_data)
            if mob_vnum:
                test_get_mob(sock, mob_vnum)
                test_delete_mob(sock, mob_vnum)

            print("\n" + "="*60)
            print("TESTING CREATE/DELETE OBJECT")
            print("="*60)

            obj_data = {
                "vnum": 390,  # Explicit vnum to avoid "no available vnums"
                "short_desc": "тестовый объект для создания",
                "aliases": "тестобъект"
            }
            # Try zone 10 (less likely to be full)
            obj_vnum = test_create_object(sock, 3, obj_data)
            if obj_vnum:
                test_get_object(sock, obj_vnum)
                test_delete_object(sock, obj_vnum)

            print("\n" + "="*60)
            print("TESTING CREATE/DELETE ROOM")
            print("="*60)

            room_data = {
                "vnum": 199,  # Explicit vnum (zone 1 range is 100-199)
                "name": "Тестовая комната для создания",
                "description": "Описание тестовой комнаты."
            }
            room_vnum = test_create_room(sock, 1, room_data)
            if room_vnum:
                test_get_room(sock, room_vnum)
                test_delete_room(sock, room_vnum)

        elif command == "full_crud_test":
            # Test list commands
            print("\n" + "="*60)
            print("TESTING LIST COMMANDS")
            print("="*60)
            test_list_zones(sock)
            test_list_mobs(sock, "1")
            test_list_objects(sock, "1")
            test_list_rooms(sock, "1")

            # Test mob CRUD
            print("\n" + "="*60)
            print("TESTING MOB CRUD")
            print("="*60)

            mob = test_get_mob(sock, 100)
            if mob.get("status") == "ok":
                data = {
                    "names": {
                        "nominative": "тестовый моб CRUD"
                    },
                    "stats": {
                        "level": 99
                    }
                }
                test_update_mob(sock, 100, data)
                test_get_mob(sock, 100)

            # Test object CRUD
            print("\n" + "="*60)
            print("TESTING OBJECT CRUD")
            print("="*60)

            obj = test_get_object(sock, 100)
            if obj.get("status") == "ok":
                data = {
                    "short_desc": "тестовый объект CRUD",
                    "aliases": "тест объект crud"
                }
                test_update_object(sock, 100, data)
                test_get_object(sock, 100)

            # Test room CRUD
            print("\n" + "="*60)
            print("TESTING ROOM CRUD")
            print("="*60)

            room = test_get_room(sock, 100)
            if room.get("status") == "ok":
                data = {
                    "name": "Тестовая комната CRUD",
                    "description": "Описание тестовой комнаты для CRUD операций."
                }
                test_update_room(sock, 100, data)
                test_get_room(sock, 100)

        elif command == "comprehensive_test":
            print("\n" + "="*60)
            print("COMPREHENSIVE FIELD TESTING")
            print("="*60)
            
            # Test mob comprehensive
            print("\n--- MOB COMPREHENSIVE TEST ---")
            test_comprehensive_mob(sock, 100)
            
            # Test object comprehensive
            print("\n--- OBJECT COMPREHENSIVE TEST ---")
            test_comprehensive_object(sock, 100)
            
            # Test room comprehensive (use 101 which exists)
            print("\n--- ROOM COMPREHENSIVE TEST ---")
            test_comprehensive_room(sock, 101)
            
            print("\n" + "="*60)
            print("COMPREHENSIVE TESTING COMPLETE")
            print("="*60)

        else:
            print(f"Error: Unknown command '{command}'")
            print("\nAvailable commands:")
            print("  get_mob <vnum>")
            print("  update_mob <vnum>")
            print("  list_mobs <zone>")
            print("  get_object <vnum>")
            print("  update_object <vnum>")
            print("  list_objects <zone>")
            print("  get_room <vnum>")
            print("  update_room <vnum>")
            print("  list_rooms <zone>")
            print("  list_zones")
            print("  create_delete_test")
            print("  full_crud_test")
            sys.exit(1)

    finally:
        sock.close()

if __name__ == "__main__":
    main()
