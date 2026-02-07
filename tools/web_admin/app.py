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
    """Main page - dashboard with statistics"""
    # Get MUD client with session credentials
    mud = get_mud_client()

    # Get statistics from Admin API
    stats_resp = mud.get_stats()
    players_resp = mud.get_players()

    stats = {}
    if stats_resp.get('status') == 'ok':
        stats['zones'] = stats_resp.get('zones', 0)
        stats['mobs'] = stats_resp.get('mobs', 0)
        stats['objects'] = stats_resp.get('objects', 0)
        stats['rooms'] = stats_resp.get('rooms', 0)
        stats['triggers'] = stats_resp.get('triggers', 0)
    else:
        stats['zones'] = 'N/A'
        stats['mobs'] = 'N/A'
        stats['objects'] = 'N/A'
        stats['rooms'] = 'N/A'
        stats['triggers'] = 'N/A'

    if players_resp.get('status') == 'ok':
        stats['players_online'] = players_resp.get('count', 0)
        stats['players_list'] = players_resp.get('players', [])
    else:
        stats['players_online'] = 0
        stats['players_list'] = []

    return render_template('index.html', stats=stats)


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


@app.route('/zones/<int:vnum>/edit', methods=['GET', 'POST'])
@login_required
def zone_edit(vnum):
    """Edit zone"""
    mud = get_mud_client()

    if request.method == 'POST':
        # Update zone
        data = {
            'name': request.form.get('name'),
            'comment': request.form.get('comment'),
            'author': request.form.get('author'),
            'level': int(request.form.get('level', 0)),
            'lifespan': int(request.form.get('lifespan', 0)),
        }
        resp = mud.update_zone(vnum, data)
        if resp.get('status') == 'ok':
            return redirect(url_for('zone_detail', vnum=vnum))
        else:
            return render_template('error.html', error=resp.get('error'))
    
    # GET - show edit form
    resp = mud.get_zone(vnum)
    if resp.get('status') == 'ok':
        zone = resp.get('zone', {})
        return render_template('zone_edit.html', zone=zone)
    else:
        return render_template('error.html', error=resp.get('error'))


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
        return render_template('mob_detail.html', mob=mob)
    else:
        return render_template('error.html', error=resp.get('error', 'Mob not found'))


@app.route('/mob/<int:vnum>/edit', methods=['GET', 'POST'])
@login_required
def mob_edit(vnum):
    """Edit mob"""
    mud = get_mud_client()

    if request.method == 'POST':
        # Update mob
        data = {
            'stats': {
                'level': int(request.form.get('level', 0))
            }
        }
        resp = mud.update_mob(vnum, data)
        if resp.get('status') == 'ok':
            return redirect(url_for('mob_detail', vnum=vnum))
        else:
            return render_template('error.html', error=resp.get('error'))

    # GET - show edit form
    resp = mud.get_mob(vnum)
    if resp.get('status') == 'ok':
        mob = resp.get('mob', {})
        return render_template('mob_edit.html', mob=mob)
    else:
        return render_template('error.html', error=resp.get('error'))


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


if __name__ == '__main__':
    # Get host and port from environment or use defaults
    HOST = os.environ.get('FLASK_HOST', '127.0.0.1')
    PORT = int(os.environ.get('FLASK_PORT', 5000))

    print("=" * 60)
    print("Bylins MUD Web Admin Interface")
    print("=" * 60)
    print(f"Socket path: {SOCKET_PATH}")
    print(f"Starting server on http://{HOST}:{PORT}")
    print("=" * 60)
    app.run(debug=True, host=HOST, port=PORT)
