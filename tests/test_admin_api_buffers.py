#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Тесты для проверки корректности обработки буферов в Admin API"""

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
    while b'\n' not in buffer:
        data = sock.recv(4096)
        if not data:
            raise ConnectionError("Connection closed")
        buffer += data

    line, _ = buffer.split(b'\n', 1)
    return json.loads(line.decode('utf-8'))

def test_multiple_commands_in_sequence():
    """Тест: несколько команд подряд - проверка, что буфер не загрязняется"""
    print("\n" + "="*60)
    print("TEST: Multiple commands in sequence (buffer contamination)")
    print("="*60)

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.settimeout(5)
    sock.connect(SOCKET_PATH)

    # Read welcome
    buffer = b''
    while b'\n' not in buffer:
        buffer += sock.recv(4096)

    # Auth
    response = send_command(sock, "auth", username="TestUser", password="test")
    assert response.get("status") == "ok", f"Auth failed: {response}"
    print("✓ Authenticated")

    # Send multiple commands in sequence
    commands = [
        ("list_zones", {}),
        ("list_mobs", {"zone": "1"}),
        ("list_objects", {"zone": "1"}),
        ("list_rooms", {"zone": "1"}),
        ("list_zones", {}),  # Repeat
        ("list_mobs", {"zone": "1"}),  # Repeat
    ]

    for cmd_name, cmd_args in commands:
        response = send_command(sock, cmd_name, **cmd_args)

        if response.get("status") != "ok":
            print(f"✗ Command '{cmd_name}' failed: {response.get('error')}")
            sock.close()
            return False

        # Check response has expected structure
        if cmd_name == "list_zones":
            assert "zones" in response, f"Missing 'zones' in response"
            print(f"✓ {cmd_name}: {len(response['zones'])} zones")
        elif cmd_name == "list_mobs":
            assert "mobs" in response, f"Missing 'mobs' in response"
            print(f"✓ {cmd_name}: {len(response['mobs'])} mobs")
        elif cmd_name == "list_objects":
            assert "objects" in response, f"Missing 'objects' in response"
            print(f"✓ {cmd_name}: {len(response['objects'])} objects")
        elif cmd_name == "list_rooms":
            assert "rooms" in response, f"Missing 'rooms' in response"
            print(f"✓ {cmd_name}: {len(response['rooms'])} rooms")

    sock.close()
    print("\n✅ BUFFER CONTAMINATION TEST PASSED")
    return True

def test_large_and_small_commands():
    """Тест: чередование больших и малых команд"""
    print("\n" + "="*60)
    print("TEST: Large and small commands alternating")
    print("="*60)

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.settimeout(5)
    sock.connect(SOCKET_PATH)

    # Read welcome
    buffer = b''
    while b'\n' not in buffer:
        buffer += sock.recv(4096)

    # Auth
    response = send_command(sock, "auth", username="TestUser", password="test")
    assert response.get("status") == "ok"
    print("✓ Authenticated")

    # Small command
    response = send_command(sock, "list_zones")
    assert response.get("status") == "ok"
    print(f"✓ Small command (list_zones): {len(response['zones'])} zones")

    # Large command (list_mobs returns ~6KB)
    response = send_command(sock, "list_mobs", zone="1")
    assert response.get("status") == "ok"
    print(f"✓ Large command (list_mobs): {len(response['mobs'])} mobs")

    # Small command again
    response = send_command(sock, "list_zones")
    assert response.get("status") == "ok"
    print(f"✓ Small command (list_zones): {len(response['zones'])} zones")

    # Another large command
    response = send_command(sock, "list_rooms", zone="1")
    assert response.get("status") == "ok"
    print(f"✓ Large command (list_rooms): {len(response['rooms'])} rooms")

    sock.close()
    print("\n✅ LARGE/SMALL ALTERNATING TEST PASSED")
    return True

def test_rapid_fire():
    """Тест: быстрая отправка команд"""
    print("\n" + "="*60)
    print("TEST: Rapid fire commands")
    print("="*60)

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.settimeout(5)
    sock.connect(SOCKET_PATH)

    # Read welcome
    buffer = b''
    while b'\n' not in buffer:
        buffer += sock.recv(4096)

    # Auth
    response = send_command(sock, "auth", username="TestUser", password="test")
    assert response.get("status") == "ok"
    print("✓ Authenticated")

    # Send 10 commands rapidly
    for i in range(10):
        response = send_command(sock, "list_zones")
        assert response.get("status") == "ok", f"Iteration {i} failed"
        zones_count = len(response['zones'])
        print(f"✓ Iteration {i+1}/10: {zones_count} zones")

        # All iterations should return same count
        if i == 0:
            expected_count = zones_count
        else:
            assert zones_count == expected_count, f"Zone count mismatch: {zones_count} != {expected_count}"

    sock.close()
    print("\n✅ RAPID FIRE TEST PASSED")
    return True

def main():
    try:
        all_passed = True

        # Test 1: Multiple commands in sequence
        if not test_multiple_commands_in_sequence():
            all_passed = False

        # Test 2: Large and small commands alternating
        if not test_large_and_small_commands():
            all_passed = False

        # Test 3: Rapid fire commands
        if not test_rapid_fire():
            all_passed = False

        if all_passed:
            print("\n" + "="*60)
            print("✅ ALL BUFFER TESTS PASSED")
            print("="*60)
            sys.exit(0)
        else:
            print("\n" + "="*60)
            print("❌ SOME BUFFER TESTS FAILED")
            print("="*60)
            sys.exit(1)

    except Exception as e:
        print(f"\n❌ TEST SUITE FAILED WITH EXCEPTION: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
