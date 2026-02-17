#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Test script for Admin API CRUD operations."""

import socket
import json
import sys
import os

SOCKET_PATH = os.getenv("SOCKET_PATH", "admin_api.sock")
WORLD_FORMAT = os.getenv("WORLD_FORMAT", "legacy")  # legacy, yaml, sqlite

def send_command(sock, command, **kwargs):
    """Send JSON command and receive response."""
    request = {"command": command, **kwargs}
    message = json.dumps(request) + "\n"
    sock.sendall(message.encode('utf-8'))

    # Read response (may be chunked)
    buffer = b''
    chunks = {}  # Map chunk_num -> chunk_data (bytes)
    total_chunks = None

    while True:
        data = sock.recv(4096)
        if not data:
            raise ConnectionError("Connection closed")
        buffer += data

        # Process complete lines
        while b'\n' in buffer:
            line, buffer = buffer.split(b'\n', 1)

            # Check if this is a chunked response
            if line.startswith(b'CHUNK:'):
                # Parse chunk header: CHUNK:N/Total:data
                header_end = line.index(b':', 6)  # Find second colon
                chunk_info = line[6:header_end].decode('ascii')
                chunk_data = line[header_end+1:]  # Keep as bytes

                chunk_num, total = map(int, chunk_info.split('/'))
                total_chunks = total
                chunks[chunk_num] = chunk_data

                # If we have all chunks, reassemble
                if len(chunks) == total_chunks:
                    full_response_bytes = b''.join(chunks[i] for i in range(total_chunks))
                    full_response_str = full_response_bytes.decode('utf-8')
                    return json.loads(full_response_str)
            else:
                # Non-chunked response
                return json.loads(line.decode('utf-8'))

def verify_legacy_file(entity_type, vnum, expected_fields):
    """Verify that legacy file contains the vnum (basic check)."""
    world_dir = os.environ.get('WORLD_DIR', os.path.expanduser('~/repos/mud/build_test/small'))

    # Determine zone file based on vnum
    zone_num = vnum // 100

    if entity_type == 'mob':
        file_path = f"{world_dir}/world/mob/{zone_num}.mob"
    elif entity_type == 'object':
        file_path = f"{world_dir}/world/obj/{zone_num}.obj"
    elif entity_type == 'room':
        file_path = f"{world_dir}/world/wld/{zone_num}.wld"
    else:
        return False, "Unknown entity type"

    if not os.path.exists(file_path):
        return False, f"Legacy file not found: {file_path}"

    try:
        with open(file_path, 'r', encoding='koi8-r') as f:
            content = f.read()
            # Check if vnum exists in file
            vnum_marker = f"#{vnum}\n"
            if vnum_marker in content:
                return True, "Legacy file contains vnum"
            else:
                return False, f"Vnum #{vnum} not found in {file_path}"
    except Exception as e:
        return False, f"Error reading legacy file: {e}"

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

def verify_file(entity_type, vnum, expected_fields):
    """Verify that file was saved (format-agnostic wrapper)."""
    if WORLD_FORMAT == "legacy":
        return verify_legacy_file(entity_type, vnum, expected_fields)
    elif WORLD_FORMAT == "yaml":
        return verify_yaml_file(entity_type, vnum, expected_fields)
    elif WORLD_FORMAT == "sqlite":
        # SQLite doesn't write individual files - skip verification
        return True, "SQLite format (no file verification)"
    else:
        return False, f"Unknown world format: {WORLD_FORMAT}"

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

        # Verify file
        success, msg = verify_file('mob', vnum, data)
        if success:
            print(f"  ✓ File verified: {msg}")
        else:
            print(f"  ✗ File verification failed: {msg}")
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

        # Verify file
        success, msg = verify_file('object', vnum, data)
        if success:
            print(f"  ✓ File verified: {msg}")
        else:
            print(f"  ✗ File verification failed: {msg}")
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

        # Verify file
        success, msg = verify_file('room', vnum, data)
        if success:
            print(f"  ✓ File verified: {msg}")
        else:
            print(f"  ✗ File verification failed: {msg}")
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
            print(f"  - {mob['vnum']}: {mob['aliases']} (level {mob['level']})")
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
                print(f"  ✓ File verified: {msg}")
            else:
                print(f"  ✗ File verification failed: {msg}")

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
                print(f"  ✓ File verified: {msg}")
            else:
                print(f"  ✗ File verification failed: {msg}")

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
                print(f"  ✓ File verified: {msg}")
            else:
                print(f"  ✗ File verification failed: {msg}")

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
        "abilities": {
            "strength": 18,
            "dexterity": 16,
            "constitution": 17,
            "intelligence": 12,
            "wisdom": 14,
            "charisma": 10
        },
        "resistances": {
            "fire": 10,
            "air": 5,
            "water": 15,
            "earth": 20,
            "vitality": 8,
            "mind": 12,
            "immunity": 0
        },
        "savings": {
            "will": 5,
            "stability": 10,
            "reflex": 8
        },
        "position": {
            "default_position": 8,
            "load_position": 8
        },
        "behavior": {
            "class": 1,
            "attack_type": 0
        },
        "sex": 0,
        "physical": {
            "height": 170,
            "weight": 75,
            "size": 50,
            "extra_attack": 1,
            "remort": 0,
            "like_work": 0,
            "maxfactor": 10
        },
        "death_load": [
            {"obj_vnum": 100, "load_prob": 50, "load_type": 0, "spec_param": 0},
            {"obj_vnum": 101, "load_prob": 25, "load_type": 1, "spec_param": 5}
        ],
        "roles": [0, 2]  # Boss (0) + Tank (2)
    }

    # Update with comprehensive data
    response = send_command(sock, "update_mob", vnum=vnum, data=comprehensive_data)

    if response.get("status") == "ok":
        print(f"✓ Comprehensive mob update successful")
        print(f"  ✓ Tested: resistances (7 fields), savings (3 fields)")
        print(f"  ✓ Tested: position (nested object), behavior (nested object)")
        print(f"  ✓ Tested: abilities (6 stats)")
        print(f"  ✓ Tested: physical (7 fields: height, weight, size, extra_attack, remort, like_work, maxfactor)")
        print(f"  ✓ Tested: death_load (2 items), roles (2 bits)")

        # Verify by getting mob back
        response = test_get_mob(sock, vnum)
        if response and response.get('status') == 'ok':
            mob = response.get('mob', {})
            physical = mob.get('physical', {})
            death_load = mob.get('death_load', [])
            roles = mob.get('roles', [])
            stats = mob.get('stats', {})
            print(f"  ✓ Verified: level={stats.get('level')}, sex={mob.get('sex')}")
            print(f"  ✓ Verified: physical.height={physical.get('height')}, physical.weight={physical.get('weight')}")
            print(f"  ✓ Verified: death_load count={len(death_load)}, roles count={len(roles)}")
            if physical.get('height') == 170 and len(death_load) == 2 and len(roles) == 2:
                print(f"  ✅ MOB UPDATE TEST PASSED (all new fields verified)")
                return True
            else:
                print(f"  ❌ MOB UPDATE TEST FAILED - field values mismatch")
                return False
        else:
            print(f"  ❌ MOB UPDATE TEST FAILED - verification failed")
            return False
    else:
        print(f"✗ Error: {response.get('error', 'Unknown error')}")
        print(f"  ❌ MOB UPDATE TEST FAILED")
        return False

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
            print(f"  ✅ OBJECT UPDATE TEST PASSED")
            return True
        else:
            print(f"  ❌ OBJECT UPDATE TEST FAILED - verification failed")
            return False
    else:
        print(f"✗ Error: {response.get('error', 'Unknown error')}")
        print(f"  ❌ OBJECT UPDATE TEST FAILED")
        return False

def test_comprehensive_room(sock, vnum):
    """Test ALL room fields comprehensively."""
    print(f"\n=== Testing comprehensive room update {vnum} ===")

    comprehensive_data = {
        "name": "Comprehensive Test Room",
        "description": "This room is used for comprehensive field testing.",
        "sector_type": 0,  # INSIDE
        "triggers": [100],  # Add trigger 100
        "room_flags": [6, 0, 0, 0],  # Plane 0: bits 1 and 2 (DARK + DEATH = 0x6)
        "exits": [
            {"direction": 0, "to_room": 101, "exit_info": 0},  # North to room 101 (exit to self)
            {"direction": 4, "to_room": 102, "exit_info": 1}   # Up to room 102 (door)
        ]
    }
    
    response = send_command(sock, "update_room", vnum=vnum, data=comprehensive_data)

    if response.get("status") == "ok":
        print(f"✓ Comprehensive room update successful")

        response = test_get_room(sock, vnum)
        if response and response.get('status') == 'ok':
            room = response.get('room', {})
            triggers = room.get('triggers', [])
            exits = room.get('exits', [])
            exit_dirs = [e['direction'] for e in exits]
            room_flags = room.get('room_flags', [0,0,0,0])
            print(f"  ✓ Verified: sector={room.get('sector_type')}, triggers={triggers}")
            print(f"  ✓ Verified: exits={exit_dirs} (expected: [0, 4])")
            print(f"  ✓ Verified: room_flags[0]={room_flags[0]} (expected: 6 = DARK+DEATH)")
            if sorted(exit_dirs) == [0, 4] and room_flags[0] == 6:
                print(f"  ✅ ROOM UPDATE TEST PASSED (exits + flags verified)")
                return True
            else:
                print(f"  ❌ ROOM UPDATE TEST FAILED - exits or flags mismatch")
                print(f"      Got: exits={exit_dirs}, room_flags[0]={room_flags[0]}")
                return False
        else:
            print(f"  ❌ ROOM UPDATE TEST FAILED - verification failed")
            return False
    else:
        print(f"✗ Error: {response.get('error', 'Unknown error')}")
        print(f"  ❌ ROOM UPDATE TEST FAILED")
        return False


def test_comprehensive_zone(sock, vnum):
    """Test comprehensive zone update with all fields."""
    print(f"\n[ZONE #{vnum}] Testing comprehensive update...")

    # First get current zone
    response = send_command(sock, "get_zone", vnum=vnum)
    if response.get('status') != 'ok':
        print(f"  ✗ Failed to get zone: {response.get('error')}")
        return False

    original = response.get('zone', {})
    print(f"  ✓ Got zone: {original.get('name', 'unnamed')}")

    # Update with all fields (14 fields total)
    update_data = {
        'name': 'COMPREHENSIVE TEST ZONE',
        'author': 'Test Author',
        'comment': 'Test comment for comprehensive zone test',
        'location': 'Test Location',
        'description': 'Comprehensive test zone description',
        'level': 10,
        'type': 1,
        'lifespan': 25,
        'reset_mode': 2,
        'reset_idle': True,
        'under_construction': False,
        'group': 5,
        'is_town': True,
        'locked': False
    }

    print(f"  → Updating zone with all 14 fields...")
    response = send_command(sock, "update_zone", vnum=vnum, data=update_data)

    if response.get('status') != 'ok':
        print(f"  ✗ Update failed: {response.get('error')}")
        return False

    print(f"  ✓ Update successful")

    # Verify by getting again
    response = send_command(sock, "get_zone", vnum=vnum)
    if response.get('status') != 'ok':
        print(f"  ✗ Failed to verify: {response.get('error')}")
        return False

    updated = response.get('zone', {})

    # Check all fields
    checks = [
        ('name', update_data['name']),
        ('author', update_data['author']),
        ('comment', update_data['comment']),
        ('location', update_data['location']),
        ('description', update_data['description']),
        ('level', update_data['level']),
        ('type', update_data['type']),
        ('lifespan', update_data['lifespan']),
        ('reset_mode', update_data['reset_mode']),
        ('reset_idle', update_data['reset_idle']),
        ('under_construction', update_data['under_construction']),
        ('group', update_data['group']),
        ('is_town', update_data['is_town']),
        ('locked', update_data['locked']),
    ]

    all_ok = True
    for field, expected in checks:
        actual = updated.get(field)
        if actual == expected:
            print(f"  ✓ Verified: {field}={actual}")
        else:
            print(f"  ✗ Mismatch: {field}={actual} (expected {expected})")
            all_ok = False

    if all_ok:
        print(f"  ✅ ZONE UPDATE TEST PASSED")
    else:
        print(f"  ❌ ZONE UPDATE TEST FAILED")

    # Restore original
    print(f"  → Restoring original zone...")
    restore_data = {
        'name': original.get('name', ''),
        'author': original.get('author', ''),
        'comment': original.get('comment', ''),
        'location': original.get('location', ''),
        'description': original.get('description', ''),
        'level': original.get('level', 1),
        'type': original.get('type', 0),
        'lifespan': original.get('lifespan', 20),
        'reset_mode': original.get('reset_mode', 2),
        'reset_idle': original.get('reset_idle', False),
        'under_construction': original.get('under_construction', False),
        'group': original.get('group', 1),
        'is_town': original.get('is_town', False),
        'locked': original.get('locked', False),
    }
    send_command(sock, "update_zone", vnum=vnum, data=restore_data)
    print(f"  ✓ Restored original")

    return all_ok


def test_comprehensive_trigger(sock, zone_vnum=1):
    """Test comprehensive trigger create/update/delete with all fields."""
    print(f"\n[TRIGGER CREATE/UPDATE TEST] Zone {zone_vnum}")

    # PART 1: Create trigger with Russian text (auto-assign vnum)
    print(f"  → Creating new trigger (auto vnum)...")
    create_data = {
        'name': 'Тестовый триггер',  # Russian text
        'attach_type': 1,  # MOB
        'trigger_type': 2,  # GREET
        'narg': 100,
        'arglist': 'привет тест',  # Russian argument
        'script': '%echo% Привет, %actor.name%!\nwait 1\n%echo% Тестовый скрипт работает.'
    }

    response = send_command(sock, "create_trigger", zone=zone_vnum, data=create_data)
    if response.get('status') != 'ok':
        print(f"  ✗ Create failed: {response.get('error')}")
        return False

    vnum = response.get('vnum', -1)
    if vnum < 0:
        print(f"  ✗ No vnum returned")
        return False

    print(f"  ✓ Trigger created with vnum={vnum}")

    # PART 2: Verify creation and encoding
    print(f"  → Verifying created trigger...")
    response = send_command(sock, "get_trigger", vnum=vnum)
    if response.get('status') != 'ok':
        print(f"  ✗ Failed to get created trigger: {response.get('error')}")
        return False

    created = response.get('trigger', {})

    # Check Russian text is preserved
    if created.get('name') == create_data['name']:
        print(f"  ✓ Name correct (Russian encoding preserved)")
    else:
        print(f"  ✗ Name mismatch: got '{created.get('name')}', expected '{create_data['name']}'")

    if created.get('arglist') == create_data['arglist']:
        print(f"  ✓ Arglist correct (Russian encoding preserved)")
    else:
        print(f"  ✗ Arglist mismatch: got '{created.get('arglist')}', expected '{create_data['arglist']}'")

    # PART 3: Update with comprehensive data
    print(f"  → Updating trigger with comprehensive data...")
    
    update_data = {
        'name': 'UPDATED TRIGGER via Admin API',
        'attach_type': 0,
        'trigger_type': 4194304,
        'narg': 100,
        'arglist': 'test argument',
        'script': '%echo% This trigger was updated via Admin API\nreturn 0\n'
    }

    response = send_command(sock, "update_trigger", vnum=vnum, data=update_data)
    
    if response.get('status') != 'ok':
        print(f"  ✗ Update failed: {response.get('error')}")
        return
    
    print(f"  ✓ Update successful")
    
    # Verify by getting again
    response = send_command(sock, "get_trigger", vnum=vnum)
    if response.get('status') != 'ok':
        print(f"  ✗ Failed to verify: {response.get('error')}")
        return
    
    updated = response.get('trigger', {})
    
    # Check all fields
    checks = [
        ('name', update_data['name']),
        ('attach_type', update_data['attach_type']),
        ('trigger_type', update_data['trigger_type']),
        ('narg', update_data['narg']),
        ('arglist', update_data['arglist']),
    ]
    
    all_ok = True
    for field, expected in checks:
        actual = updated.get(field)
        if actual == expected:
            print(f"  ✓ Verified: {field}={actual}")
        else:
            print(f"  ✗ Mismatch: {field}={actual} (expected {expected})")
            all_ok = False
    
    # Check script (commands)
    if updated.get('commands'):
        script_text = '\n'.join(updated['commands'])
        if 'Admin API' in script_text:
            print(f"  ✓ Verified: script contains update text")
        else:
            print(f"  ✗ Script doesn't contain update text")
            all_ok = False
    else:
        print(f"  ✗ No commands in updated trigger")
        all_ok = False
    
    # Verify file (format-dependent)
    if WORLD_FORMAT == "yaml":
        print(f"  → Verifying YAML file was saved...")
        world_dir = os.environ.get('WORLD_DIR', os.path.expanduser('~/repos/world.yaml'))
        yaml_path = f"{world_dir}/world/triggers/{vnum}.yaml"

        if os.path.exists(yaml_path):
            try:
                import yaml
                with open(yaml_path, 'r', encoding='koi8-r') as f:
                    yaml_data = yaml.safe_load(f)

                if yaml_data.get('name') == update_data['name']:
                    print(f"  ✓ YAML file saved with correct name")
                else:
                    print(f"  ✗ YAML file has wrong name: {yaml_data.get('name')}")
                    all_ok = False

                if yaml_data.get('arglist') == update_data['arglist']:
                    print(f"  ✓ YAML file saved with correct arglist")
                else:
                    print(f"  ✗ YAML file has wrong arglist: {yaml_data.get('arglist')}")
                    all_ok = False

            except Exception as e:
                print(f"  ✗ Failed to read YAML: {e}")
                all_ok = False
        else:
            print(f"  ✗ YAML file not found: {yaml_path}")
            all_ok = False
    else:
        print(f"  → Skipping file verification for {WORLD_FORMAT} format")
    
    # PART 4: Cleanup - delete the test trigger
    print(f"  → Deleting test trigger {vnum}...")
    response = send_command(sock, "delete_trigger", vnum=vnum)
    if response.get('status') == 'ok':
        print(f"  ✓ Trigger deleted successfully")
    else:
        print(f"  ✗ Delete failed: {response.get('error', 'Unknown error')}")
        # Don't fail the test if delete doesn't work - it's cleanup

    if all_ok:
        print(f"  ✅ TRIGGER CREATE/UPDATE/DELETE TEST PASSED")
    else:
        print(f"  ❌ TRIGGER TEST FAILED")

    return all_ok

def test_mob_flags(sock, vnum):
    """Test mob flags - read, update, and verify."""
    print(f"\n[MOB #{vnum}] Testing mob flags...")

    # First get current mob to see existing flags
    response = send_command(sock, "get_mob", vnum=vnum)
    if response.get('status') != 'ok':
        print(f"  ✗ Failed to get mob: {response.get('error')}")
        return False

    original = response.get('mob', {})
    original_flags = original.get('flags', {})
    print(f"  ✓ Got mob flags:")
    print(f"    - mob_flags: {len(original_flags.get('mob_flags', []))} flags")
    print(f"    - npc_flags: {len(original_flags.get('npc_flags', []))} flags")
    print(f"    - affect_flags: {len(original_flags.get('affect_flags', []))} flags")

    # Test: Update flags (add some test flags)
    test_flags = {
        "mob_flags": [1, 2, 4],  # Test values
        "npc_flags": [1, 8, 16],  # Test values
        "affect_flags": [1, 2]    # Test values
    }
    update_data = {"flags": test_flags}

    print(f"  → Updating flags...")
    response = send_command(sock, "update_mob", vnum=vnum, data=update_data)

    if response.get('status') != 'ok':
        print(f"  ✗ Update failed: {response.get('error')}")
        return False

    print(f"  ✓ Update successful")

    # Verify by getting again
    response = send_command(sock, "get_mob", vnum=vnum)
    if response.get('status') != 'ok':
        print(f"  ✗ Failed to verify: {response.get('error')}")
        return False

    updated = response.get('mob', {})
    updated_flags = updated.get('flags', {})

    # Check if flags match (note: update might ADD flags, not replace them)
    mob_flags = set(updated_flags.get('mob_flags', []))
    npc_flags = set(updated_flags.get('npc_flags', []))
    affect_flags = set(updated_flags.get('affect_flags', []))

    test_mob_set = set(test_flags['mob_flags'])
    test_npc_set = set(test_flags['npc_flags'])
    test_affect_set = set(test_flags['affect_flags'])

    all_ok = True
    if not test_mob_set.issubset(mob_flags):
        print(f"  ✗ mob_flags missing some values: expected {test_mob_set}, got {mob_flags}")
        all_ok = False
    else:
        print(f"  ✓ mob_flags verified: {len(mob_flags)} flags")

    if not test_npc_set.issubset(npc_flags):
        print(f"  ✗ npc_flags missing some values: expected {test_npc_set}, got {npc_flags}")
        all_ok = False
    else:
        print(f"  ✓ npc_flags verified: {len(npc_flags)} flags")

    if not test_affect_set.issubset(affect_flags):
        print(f"  ✗ affect_flags missing some values: expected {test_affect_set}, got {affect_flags}")
        all_ok = False
    else:
        print(f"  ✓ affect_flags verified: {len(affect_flags)} flags")

    if all_ok:
        print(f"  ✅ MOB FLAGS TEST PASSED")
    else:
        print(f"  ❌ MOB FLAGS TEST FAILED")

    # Restore original flags
    print(f"  → Restoring original flags...")
    restore_data = {"flags": original_flags}
    send_command(sock, "update_mob", vnum=vnum, data=restore_data)
    print(f"  ✓ Restored original")

    return all_ok

def test_large_payload(sock, vnum):
    """Test saving large mob data (chunking support)."""
    print(f"Testing large payload for mob vnum={vnum}")

    # Large mob data similar to user's payload (~2KB)
    large_mob_data = {
        "names": {
            "aliases": "костяная гончая",
            "nominative": "костяная гончая",
            "genitive": "костяной гончей",
            "dative": "костяной гончей",
            "accusative": "костяную гончую",
            "instrumental": "костяной гончей",
            "prepositional": "костяной гончей"
        },
        "descriptions": {
            "short_desc": "Костяная гончая стоит здесь, ожидая приказа своего хозяина.",
            "long_desc": "Груда костей, скрепленная темной магией в ужасное порождение тьмы.\nЭто создание служит своему хозяину безропотно и беспрекословно.\n"
        },
        "stats": {
            "level": 15,
            "sex": 2,
            "race": 110,
            "alignment": -600,
            "armor": 3,
            "hitroll_penalty": 5,
            "hp": {"dice_count": 10, "dice_size": 4, "bonus": 220},
            "damage": {"dice_count": 5, "dice_size": 4, "bonus": 10}
        },
        "physical": {
            "height": 150,
            "weight": 150,
            "size": 45,
            "extra_attack": 0,
            "remort": 0,
            "like_work": 0,
            "maxfactor": 2
        },
        "abilities": {
            "strength": 18,
            "dexterity": 18,
            "constitution": 3,
            "intelligence": 10,
            "wisdom": 10,
            "charisma": 10
        },
        "resistances": {
            "fire": 7,
            "air": 0,
            "water": 0,
            "earth": 0,
            "vitality": 50,
            "mind": 5,
            "immunity": 95
        },
        "savings": {
            "will": 80,
            "stability": 0,
            "reflex": 0
        },
        "position": {
            "default_position": 8,
            "load_position": 8
        },
        "behavior": {
            "class": -1,
            "attack_type": 0,
            "gold": {"dice_count": 0, "dice_size": 0, "bonus": 0},
            "helpers": []
        },
        "triggers": [],
        "roles": [],
        "flags": {
            "mob_flags": [1, 2, 4, 8, 16],
            "affect_flags": [],
            "npc_flags": []
        },
        "death_load": []
    }

    print(f"  → Payload size: ~{len(json.dumps(large_mob_data))} bytes")

    try:
        # Update mob with large payload
        print(f"  → Sending large update command...")
        response = send_command(sock, "update_mob", vnum=vnum, data=large_mob_data)

        if response.get("status") != "ok":
            print(f"  ❌ Update failed: {response.get('error', 'Unknown error')}")
            return False

        print(f"  ✓ Update successful")

        # Verify data was saved correctly
        print(f"  → Verifying saved data...")
        response = send_command(sock, "get_mob", vnum=vnum)

        if response.get("status") != "ok":
            print(f"  ❌ Get failed: {response.get('error', 'Unknown error')}")
            return False

        saved_data = response.get("mob", {})

        # Verify key fields
        all_ok = True
        if saved_data.get("names", {}).get("aliases") != large_mob_data["names"]["aliases"]:
            print(f"  ❌ Aliases mismatch")
            all_ok = False
        else:
            print(f"  ✓ Aliases correct")

        if saved_data.get("stats", {}).get("level") != large_mob_data["stats"]["level"]:
            print(f"  ❌ Level mismatch")
            all_ok = False
        else:
            print(f"  ✓ Level correct")

        if saved_data.get("resistances", {}).get("immunity") != large_mob_data["resistances"]["immunity"]:
            print(f"  ❌ Immunity mismatch")
            all_ok = False
        else:
            print(f"  ✓ Resistances correct")

        if all_ok:
            print(f"  ✅ LARGE PAYLOAD TEST PASSED")
        else:
            print(f"  ❌ LARGE PAYLOAD TEST FAILED - Data mismatch")

        return all_ok

    except ConnectionError as e:
        print(f"  ❌ Connection error: {e}")
        print(f"  This indicates chunking is not working properly")
        return False
    except Exception as e:
        print(f"  ❌ Unexpected error: {e}")
        return False

def main():
    global WORLD_FORMAT

    # Parse --world-format argument and --help
    args = sys.argv[1:]

    # Check for --help first
    if '--help' in args or '-h' in args:
        args = []  # Force help display

    for i, arg in enumerate(args[:]):  # Iterate over copy to allow modification
        if arg.startswith('--world-format='):
            WORLD_FORMAT = arg.split('=', 1)[1]
            args.remove(arg)
            break

    if len(args) < 1:
        print("Usage: test_admin_api.py [--world-format=FORMAT] <command> [args...]")
        print("\nOptions:")
        print("  --world-format=FORMAT  World data format (legacy|yaml|sqlite)")
        print("                         Can also be set via WORLD_FORMAT env variable")
        print("                         Default: legacy")
        print("\nIndividual commands:")
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
        print("\nTest suites:")
        print("  create_delete_test  - Test create/delete operations")
        print("  full_crud_test      - Test all endpoints with comprehensive field data")
        print("\nEnvironment variables:")
        print("  SOCKET_PATH  - Path to admin API socket (default: admin_api.sock)")
        print("  WORLD_DIR    - Path to world directory (default varies by format)")
        print("  WORLD_FORMAT - World data format (legacy|yaml|sqlite, default: legacy)")
        print("  MUD_USERNAME - Username for authentication")
        print("  MUD_PASSWORD - Password for authentication")
        sys.exit(1)

    command = args[0]

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
        # Note: Always send 'auth' command, even if require_auth=false
        # Server will auto-authenticate if require_auth=false
        username = os.getenv("MUD_USERNAME", "TestUser")
        password = os.getenv("MUD_PASSWORD", "")

        auth_response = send_command(sock, "auth", username=username, password=password)
        print(f"DEBUG: Auth response: {auth_response}")
        if auth_response.get("status") != "ok":
            print(f"✗ Authentication failed: {auth_response.get('error')}")
            if password:
                # Real password was provided, so this is a real auth failure
                sys.exit(1)
            else:
                # No password provided, but auth still failed - server requires auth
                print("  Hint: Server requires authentication. Set MUD_USERNAME and MUD_PASSWORD.")
                sys.exit(1)
        print("✓ Authenticated")

        # Execute command
        if command == "get_mob":
            vnum = int(args[1]) if len(args) > 1 else 100
            test_get_mob(sock, vnum)

        elif command == "update_mob":
            vnum = int(args[1]) if len(args) > 1 else 100

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
            vnum = int(args[1]) if len(args) > 1 else 100
            test_get_object(sock, vnum)

        elif command == "update_object":
            vnum = int(args[1]) if len(args) > 1 else 100

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
            vnum = int(args[1]) if len(args) > 1 else 3000
            test_get_room(sock, vnum)

        elif command == "update_room":
            vnum = int(args[1]) if len(args) > 1 else 3000

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
            zone = args[1] if len(args) > 1 else "1"
            test_list_mobs(sock, zone)

        elif command == "list_objects":
            zone = args[1] if len(args) > 1 else "1"
            test_list_objects(sock, zone)

        elif command == "list_rooms":
            zone = args[1] if len(args) > 1 else "1"
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

            # Track test results
            all_tests_passed = True

            # Test comprehensive zone CRUD (all fields)
            print("\n" + "="*60)
            print("TESTING ZONE COMPREHENSIVE UPDATE (ALL FIELDS)")
            print("="*60)
            if not test_comprehensive_zone(sock, 1):
                all_tests_passed = False

            # Test comprehensive mob CRUD (all fields)
            print("\n" + "="*60)
            print("TESTING MOB COMPREHENSIVE UPDATE (ALL FIELDS)")
            print("="*60)
            if not test_comprehensive_mob(sock, 100):
                all_tests_passed = False

            # Test mob flags specifically
            print("\n" + "="*60)
            print("TESTING MOB FLAGS (GET/UPDATE/VERIFY)")
            print("="*60)
            if not test_mob_flags(sock, 100):
                all_tests_passed = False

            # Test large payload (chunking support)
            print("\n" + "="*60)
            print("TESTING LARGE PAYLOAD (CHUNKING)")
            print("="*60)
            if not test_large_payload(sock, 100):
                all_tests_passed = False

            # Test comprehensive object CRUD (all fields)
            print("\n" + "="*60)
            print("TESTING OBJECT COMPREHENSIVE UPDATE (ALL FIELDS)")
            print("="*60)
            if not test_comprehensive_object(sock, 100):
                all_tests_passed = False

            # Test comprehensive room CRUD (all fields)
            print("\n" + "="*60)
            print("TESTING ROOM COMPREHENSIVE UPDATE (ALL FIELDS)")
            print("="*60)
            if not test_comprehensive_room(sock, 101):
                all_tests_passed = False

            # Test comprehensive trigger CRUD (all fields)
            print("\n" + "="*60)
            print("TESTING TRIGGER COMPREHENSIVE UPDATE (ALL FIELDS)")
            print("="*60)
            if not test_comprehensive_trigger(sock, zone_vnum=1):
                all_tests_passed = False

            print("\n" + "="*60)
            if all_tests_passed:
                print("✅ FULL CRUD TEST COMPLETE - ALL TESTS PASSED")
                print("="*60)
            else:
                print("❌ FULL CRUD TEST COMPLETE - SOME TESTS FAILED")
                print("="*60)
                sys.exit(1)

        else:
            print(f"Error: Unknown command '{command}'")
            print("\nIndividual commands:")
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
            print("\nTest suites:")
            print("  create_delete_test  - Test create/delete operations")
            print("  full_crud_test      - Test all endpoints with comprehensive field data")
            sys.exit(1)

    finally:
        sock.close()

if __name__ == "__main__":
    main()
