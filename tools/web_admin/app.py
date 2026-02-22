"""
Bylins MUD Web Admin Interface
Flask application for managing MUD world via Admin API
"""
from flask import Flask, render_template, request, jsonify, redirect, url_for, session
from functools import wraps
from mud_client import MudAdminClient
import os

app = Flask(__name__)
app.config['SECRET_KEY'] = os.urandom(24)

# Get socket path from environment or use default (current directory)
SOCKET_PATH = os.environ.get('MUD_SOCKET', 'admin_api.sock')
SOCKET_PATH = os.path.expanduser(SOCKET_PATH)

# Check if socket exists
if not os.path.exists(SOCKET_PATH):
    print(f"ERROR: Socket file not found: {SOCKET_PATH}")
    print(f"Make sure MUD server is running with Admin API enabled.")
    print(f"You can override socket path with: MUD_SOCKET=/path/to/socket python3 app.py")
    exit(1)

# Helper function to get MUD client with session credentials
def get_mud_client():
    """Get MUD client with credentials from session"""
    username = session.get('username')
    password = session.get('password')

    if username and password:
        return MudAdminClient(SOCKET_PATH, username=username, password=password)
    else:
        # For backwards compatibility - try without auth
        return MudAdminClient(SOCKET_PATH)


def resolve_trigger_name(mud, vnum):
    """Fetch trigger name by vnum. Returns name string or None."""
    try:
        resp = mud.get_trigger(vnum)
        if resp.get('status') == 'ok':
            return resp.get('trigger', {}).get('name', '')
    except Exception:
        pass
    return None


def resolve_object_name(mud, vnum):
    """Fetch object short_desc by vnum. Returns name string or None."""
    try:
        resp = mud.get_object(vnum)
        if resp.get('status') == 'ok':
            obj = resp.get('object', {})
            return obj.get('short_desc', '') or obj.get('aliases', '')
    except Exception:
        pass
    return None


def resolve_room_name(mud, vnum):
    """Fetch room name by vnum. Returns name string or None."""
    try:
        resp = mud.get_room(vnum)
        if resp.get('status') == 'ok':
            return resp.get('room', {}).get('name', '')
    except Exception:
        pass
    return None


def resolve_mob_name(mud, vnum):
    """Fetch mob short name by vnum. Returns name string or None."""
    try:
        resp = mud.get_mob(vnum)
        if resp.get('status') == 'ok':
            mob = resp.get('mob', {})
            names = mob.get('names', {})
            return names.get('nominative', '') or names.get('aliases', '')
    except Exception:
        pass
    return None


def enrich_trigger_list(mud, trigger_vnums):
    """Convert list of trigger vnums to list of {vnum, name} dicts."""
    result = []
    for vnum in trigger_vnums:
        name = resolve_trigger_name(mud, vnum)
        result.append({'vnum': vnum, 'name': name or ''})
    return result


# Login required decorator
def login_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'username' not in session:
            return redirect(url_for('login'))
        return f(*args, **kwargs)
    return decorated_function


@app.route('/login', methods=['GET', 'POST'])
def login():
    """Login page"""
    if request.method == 'POST':
        username = request.form.get('username')
        password = request.form.get('password')

        # Try to authenticate with MUD
        try:
            test_client = MudAdminClient(SOCKET_PATH, username=username, password=password)
            # Test connection with real command
            response = test_client._send_command('list_zones')

            if response.get('status') == 'ok':
                # Authentication successful - save to session
                session['username'] = username
                session['password'] = password
                return redirect(url_for('index'))
            else:
                return render_template('login.html', error=f"Ошибка: {response.get('error', 'Неизвестная ошибка')}")

        except ValueError as e:
            # Authentication failed
            return render_template('login.html', error=str(e))
        except Exception as e:
            return render_template('login.html', error=f'Ошибка подключения: {str(e)}')

    return render_template('login.html')


@app.route('/logout')
def logout():
    """Logout - clear session"""
    session.clear()
    return redirect(url_for('login'))


@app.route('/')
@login_required
def index():
    """Main page"""
    mud = get_mud_client()
    stats = mud.get_stats()
    players = mud.get_players()
    return render_template('index.html', username=session.get('username'), stats=stats, players=players)


@app.route('/zones')
@login_required
def zones_list():
    """List all zones"""
    mud = get_mud_client()
    resp = mud.list_zones()
    if resp.get('status') == 'ok':
        zones = resp.get('zones', [])
        return render_template('zones_list.html', zones=zones)
    else:
        return render_template('error.html', error=resp.get('error', 'Unknown error'))


@app.route('/zones/<int:vnum>')
@login_required
def zone_detail(vnum):
    """Zone details page"""
    mud = get_mud_client()
    resp = mud.get_zone(vnum)
    if resp.get('status') == 'ok':
        zone = resp.get('zone', {})
        return render_template('zone_detail.html', zone=zone)
    else:
        return render_template('error.html', error=resp.get('error', 'Zone not found'))


@app.route('/zones/<int:vnum>/edit', methods=['GET'])
@login_required
def zone_edit(vnum):
    """Edit zone - show form"""
    mud = get_mud_client()
    resp = mud.get_zone(vnum)
    if resp.get('status') == 'ok':
        zone = resp.get('zone', {})
        return render_template('zone_edit.html', zone=zone)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/zones/<int:vnum>/update', methods=['POST'])
@login_required
def zone_update(vnum):
    """Update zone via JSON API - accepts full zone data structure"""
    mud = get_mud_client()

    # Accept JSON body from AJAX form
    data = request.get_json()
    if not data:
        return jsonify({'status': 'error', 'error': 'No JSON data received'}), 400

    resp = mud.update_zone(vnum, data)
    return jsonify(resp)


@app.route('/mobs')
@login_required
def mobs_list():
    """List mobs in zone"""
    mud = get_mud_client()
    zone = request.args.get('zone', '1')
    resp = mud.list_mobs(zone)
    if resp.get('status') == 'ok':
        mobs = resp.get('mobs', [])
        return render_template('mobs_list.html', mobs=mobs, zone=zone)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/mob/<int:vnum>')
@login_required
def mob_detail(vnum):
    """Mob detail page"""
    mud = get_mud_client()
    resp = mud.get_mob(vnum)
    if resp.get('status') == 'ok':
        mob = resp.get('mob', {})
        # Enrich triggers with names
        if mob.get('triggers'):
            mob['triggers_enriched'] = enrich_trigger_list(mud, mob['triggers'])
        # Enrich death_load with object names
        if mob.get('death_load'):
            for item in mob['death_load']:
                obj_vnum = item.get('obj_vnum')
                if obj_vnum:
                    item['obj_name'] = resolve_object_name(mud, obj_vnum) or ''
        # Enrich helpers with mob names
        if mob.get('behavior', {}).get('helpers'):
            enriched_helpers = []
            for h_vnum in mob['behavior']['helpers']:
                h_name = resolve_mob_name(mud, h_vnum) or ''
                enriched_helpers.append({'vnum': h_vnum, 'name': h_name})
            mob['behavior']['helpers_enriched'] = enriched_helpers
        return render_template('mob_detail.html', mob=mob)
    else:
        return render_template('error.html', error=resp.get('error', 'Mob not found'))


@app.route('/mob/<int:vnum>/edit', methods=['GET'])
@login_required
def mob_edit(vnum):
    """Edit mob - show form"""
    mud = get_mud_client()
    resp = mud.get_mob(vnum)
    if resp.get('status') == 'ok':
        mob = resp.get('mob', {})
        return render_template('mob_edit.html', mob=mob)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/mob/<int:vnum>/update', methods=['POST'])
@login_required
def mob_update(vnum):
    """Update mob via JSON API - accepts full nested mob data structure"""
    mud = get_mud_client()

    # Accept JSON body from AJAX form
    data = request.get_json()
    if not data:
        return jsonify({'status': 'error', 'error': 'No JSON data received'}), 400

    resp = mud.update_mob(vnum, data)
    return jsonify(resp)


@app.route('/objects')
@login_required
def objects_list():
    """List objects in zone"""
    mud = get_mud_client()
    zone = request.args.get('zone', '1')
    resp = mud.list_objects(zone)
    if resp.get('status') == 'ok':
        objects = resp.get('objects', [])
        return render_template('objects_list.html', objects=objects, zone=zone)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/object/<int:vnum>')
@login_required
def object_detail(vnum):
    """Object detail page"""
    mud = get_mud_client()
    resp = mud.get_object(vnum)
    if resp.get('status') == 'ok':
        obj = resp.get('object', {})
        # Enrich triggers with names
        if obj.get('triggers'):
            obj['triggers_enriched'] = enrich_trigger_list(mud, obj['triggers'])
        return render_template('object_detail.html', obj=obj)
    else:
        return render_template('error.html', error=resp.get('error', 'Object not found'))


@app.route('/object/<int:vnum>/edit', methods=['GET'])
@login_required
def object_edit(vnum):
    """Edit object - show form"""
    mud = get_mud_client()
    resp = mud.get_object(vnum)
    if resp.get('status') == 'ok':
        obj = resp.get('object', {})
        return render_template('object_edit.html', obj=obj)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/object/<int:vnum>/update', methods=['POST'])
@login_required
def object_update(vnum):
    """Update object via JSON API - accepts full nested object data structure"""
    mud = get_mud_client()

    # Accept JSON body from AJAX form
    data = request.get_json()
    if not data:
        return jsonify({'status': 'error', 'error': 'No JSON data received'}), 400

    resp = mud.update_object(vnum, data)
    return jsonify(resp)


@app.route('/rooms')
@login_required
def rooms_list():
    """List rooms in zone"""
    mud = get_mud_client()
    zone = request.args.get('zone', '1')
    resp = mud.list_rooms(zone)
    if resp.get('status') == 'ok':
        rooms = resp.get('rooms', [])
        return render_template('rooms_list.html', rooms=rooms, zone=zone)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/room/<int:vnum>')
@login_required
def room_detail(vnum):
    """Room detail page"""
    mud = get_mud_client()
    resp = mud.get_room(vnum)
    if resp.get('status') == 'ok':
        room = resp.get('room', {})
        # Get zone info
        zone_vnum = vnum // 100
        zone_resp = mud.get_zone(zone_vnum)
        zone = zone_resp.get('zone', {}) if zone_resp.get('status') == 'ok' else {}
        # Enrich triggers with names
        if room.get('triggers'):
            room['triggers_enriched'] = enrich_trigger_list(mud, room['triggers'])
        # Enrich exits with room names
        if room.get('exits'):
            for exit_data in room['exits']:
                to_room = exit_data.get('to_room')
                if to_room is not None and to_room >= 0:
                    exit_data['to_room_name'] = resolve_room_name(mud, to_room) or ''
        return render_template('room_detail.html', room=room, zone=zone)
    else:
        return render_template('error.html', error=resp.get('error', 'Room not found'))


@app.route('/room/<int:vnum>/edit', methods=['GET'])
@login_required
def room_edit(vnum):
    """Edit room - show form"""
    mud = get_mud_client()
    resp = mud.get_room(vnum)
    if resp.get('status') == 'ok':
        room = resp.get('room', {})
        return render_template('room_edit.html', room=room)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/room/<int:vnum>/update', methods=['POST'])
@login_required
def room_update(vnum):
    """Update room via JSON API - accepts full nested room data structure"""
    mud = get_mud_client()

    # Accept JSON body from AJAX form
    data = request.get_json()
    if not data:
        return jsonify({'status': 'error', 'error': 'No JSON data received'}), 400

    resp = mud.update_room(vnum, data)
    return jsonify(resp)


@app.route('/triggers')
@login_required
def triggers_list():
    """List triggers in zone"""
    mud = get_mud_client()
    zone = request.args.get('zone', '1')
    resp = mud.list_triggers(zone)
    if resp.get('status') == 'ok':
        triggers = resp.get('triggers', [])
        return render_template('triggers_list.html', triggers=triggers, zone=zone)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/trigger/<int:vnum>')
@login_required
def trigger_detail(vnum):
    """Trigger detail page"""
    mud = get_mud_client()
    resp = mud.get_trigger(vnum)
    if resp.get('status') == 'ok':
        trig = resp.get('trigger', {})
        return render_template('trigger_detail.html', trig=trig)
    else:
        return render_template('error.html', error=resp.get('error', 'Trigger not found'))


@app.route('/trigger/<int:vnum>/edit', methods=['GET'])
@login_required
def trigger_edit(vnum):
    """Edit trigger - show form"""
    mud = get_mud_client()
    resp = mud.get_trigger(vnum)
    if resp.get('status') == 'ok':
        trig = resp.get('trigger', {})
        return render_template('trigger_edit.html', trig=trig)
    else:
        return render_template('error.html', error=resp.get('error'))


@app.route('/trigger/<int:vnum>/update', methods=['POST'])
@login_required
def trigger_update(vnum):
    """Update trigger via JSON API"""
    mud = get_mud_client()

    # Accept JSON body from AJAX form
    data = request.get_json()
    if not data:
        return jsonify({'status': 'error', 'error': 'No JSON data received'}), 400

    resp = mud.update_trigger(vnum, data)
    return jsonify(resp)


# API endpoints for AJAX
@app.route('/api/zones')
@login_required
def api_zones():
    """API: Get zones list"""
    mud = get_mud_client()
    return jsonify(mud.list_zones())


@app.route('/api/zones/<int:vnum>')
@login_required
def api_zone(vnum):
    """API: Get zone details"""
    mud = get_mud_client()
    return jsonify(mud.get_zone(vnum))


# ===== API ENDPOINTS FOR AUTOCOMPLETE =====

@app.route('/api/search/mobs')
@login_required
def api_search_mobs():
    """Search mobs for autocomplete"""
    query = request.args.get('q', '').lower()
    zone = request.args.get('zone', type=int)
    limit = request.args.get('limit', 10, type=int)

    mud = get_mud_client()
    if not mud:
        return jsonify({'error': 'Not authenticated'}), 401

    # Get all mobs for zone or all zones
    mobs = mud.list_mobs(zone if zone else None)
    if mobs is None:
        return jsonify({'error': 'Failed to fetch mobs'}), 500

    # Filter by query
    results = []
    for mob in mobs:
        vnum = mob.get('vnum', 0)
        name = mob.get('name', '')
        aliases = mob.get('aliases', '')

        # Match by vnum, name, or aliases
        if (query in str(vnum) or
            query in name.lower() or
            query in aliases.lower()):
            results.append({
                'vnum': vnum,
                'name': name,
                'aliases': aliases
            })

        if len(results) >= limit:
            break

    return jsonify({'results': results})


@app.route('/api/search/objects')
@login_required
def api_search_objects():
    """Search objects for autocomplete"""
    query = request.args.get('q', '').lower()
    zone = request.args.get('zone', type=int)
    limit = request.args.get('limit', 10, type=int)

    mud = get_mud_client()
    if not mud:
        return jsonify({'error': 'Not authenticated'}), 401

    objects = mud.list_objects(zone if zone else None)
    if objects is None:
        return jsonify({'error': 'Failed to fetch objects'}), 500

    results = []
    for obj in objects:
        vnum = obj.get('vnum', 0)
        name = obj.get('name', '')
        aliases = obj.get('aliases', '')

        if (query in str(vnum) or
            query in name.lower() or
            query in aliases.lower()):
            results.append({
                'vnum': vnum,
                'name': name,
                'aliases': aliases
            })

        if len(results) >= limit:
            break

    return jsonify({'results': results})


@app.route('/api/search/rooms')
@login_required
def api_search_rooms():
    """Search rooms for autocomplete"""
    query = request.args.get('q', '').lower()
    zone = request.args.get('zone', type=int)
    limit = request.args.get('limit', 10, type=int)

    mud = get_mud_client()
    if not mud:
        return jsonify({'error': 'Not authenticated'}), 401

    rooms = mud.list_rooms(zone if zone else None)
    if rooms is None:
        return jsonify({'error': 'Failed to fetch rooms'}), 500

    results = []
    for room in rooms:
        vnum = room.get('vnum', 0)
        name = room.get('name', '')

        if query in str(vnum) or query in name.lower():
            results.append({
                'vnum': vnum,
                'name': name
            })

        if len(results) >= limit:
            break

    return jsonify({'results': results})


@app.route('/api/skills')
@login_required
def api_skills():
    """Get list of all skills for autocomplete"""
    # Static list of skills - можно расширить
    skills = [
        'backstab', 'bash', 'hide', 'kick', 'pick', 'punch', 'rescue', 'sneak',
        'steal', 'track', 'protect', 'sense', 'parry', 'block', 'touch',
        'looking', 'hearing', 'disarm', 'deviate', 'chopoff', 'repair', 'courage',
        'identify', 'heal', 'townportal', 'sacrifice', 'morph', 'relocate',
        'warcry', 'skills', 'manadrain', 'sideattack', 'throw', 'stupor',
        'mighthit', 'excrete', 'recall', 'makefood', 'poisoned', 'riding',
        'horse', 'horse_training', 'leadership', 'punctual', 'awake', 'createbow',
        'poisoning', 'camouflage', 'locomotion', 'skinning', 'turn_undead',
        'iron_wind', 'stun', 'make_staff', 'religious', 'make_weapon', 'make_armor',
        'make_jewel', 'transformweapon', 'drunkoff', 'make_wear', 'make_potion',
        'style_awake', 'style_parent', 'style_touch', 'style_polar', 'create_water',
        'fire', 'smithing', 'indefinite_incapacitation'
    ]

    query = request.args.get('q', '').lower()
    if query:
        results = [s for s in skills if query in s.lower()]
    else:
        results = skills

    return jsonify({'results': results[:50]})


@app.route('/api/spells')
@login_required
def api_spells():
    """Get list of all spells for autocomplete"""
    # Static list of spells - можно расширить
    spells = [
        'armor', 'bless', 'blindness', 'burden', 'cloudly', 'cure_blind', 'cure_critic',
        'cure_light', 'detect_align', 'detect_invis', 'detect_magic', 'detect_poison',
        'dispel_evil', 'dispel_good', 'earthquake', 'extra_hits', 'fireball', 'group_armor',
        'group_bless', 'harm', 'heal', 'invisible', 'lightning', 'locate_object',
        'magic_missile', 'poison', 'prot_from_evil', 'remove_curse', 'remove_poison',
        'sanctuary', 'shocking', 'sleep', 'strength', 'summon', 'word_recall',
        'remove_hold', 'power_hold', 'mass_hold', 'fly', 'broken_chains', 'noflee',
        'create_water', 'create_food', 'refresh', 'teleport', 'create_weapon',
        'fear', 'sacrifice', 'haste', 'slow', 'clone', 'protect_magic', 'cure_serious',
        'fire_shield', 'air_shield', 'ice_shield', 'group_heal', 'mass_cure', 'ressurection',
        'animate_dead', 'stoneskin', 'cloudly', 'power_silence', 'failure', 'mass_failure',
        'mass_slow', 'mass_curse', 'holystrike', 'angel', 'demon', 'hypnotic_pattern',
        'solobonus', 'expedient', 'web', 'cone_cold', 'battle', 'acid', 'repair',
        'mindless', 'prismaticaura', 'evards_hand', 'meteorstorm', 'icestorm',
        'portalstone', 'timer_repair', 'vacuum', 'sight_of_darkness', 'general_sincerity',
        'magical_gaze', 'all_seeing_eye', 'eye_of_gods', 'breathing_at_depth',
        'general_recovery', 'common_meal', 'stonehand', 'snake_eyes', 'earth_aura',
        'group_prot_from_evil', 'arrows_fire', 'arrows_water', 'arrows_earth',
        'arrows_air', 'arrows_death', 'group_strength', 'aconitum_poison', 'scopolia_poison',
        'belena_poison', 'datura_poison'
    ]

    query = request.args.get('q', '').lower()
    if query:
        results = [s for s in spells if query in s.lower()]
    else:
        results = spells

    return jsonify({'results': results[:50]})


if __name__ == '__main__':
    HOST = os.environ.get('FLASK_HOST', '127.0.0.1')
    PORT = int(os.environ.get('FLASK_PORT', 5000))
    DEBUG = os.environ.get('FLASK_DEBUG', '0').lower() in ('1', 'true', 'yes')

    print("=" * 60)
    print("Bylins MUD Web Admin Interface")
    print("=" * 60)
    print(f"Socket path: {SOCKET_PATH}")
    print(f"Starting server on http://{HOST}:{PORT}")
    print(f"Debug: {DEBUG}")
    print("=" * 60)
    app.run(debug=DEBUG, host=HOST, port=PORT)
