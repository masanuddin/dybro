import streamlit as st
import paho.mqtt.client as mqtt
import json
import threading
import time
from datetime import datetime
from streamlit_autorefresh import st_autorefresh

# ---------------- MQTT CONFIG ----------------
DEFAULT_BROKER = "broker.hivemq.com"
DEFAULT_PORT = 1883
TOPIC_DHT = "sic/dibimbing/DoaIbuMenyertai/dht"
TOPIC_IR = "sic/dibimbing/DoaIbuMenyertai/ir"
TOPIC_BUTTON = "sic/dibimbing/DoaIbuMenyertai/button"
TOPIC_CAM = "sic/dibimbing/DoaIbuMenyertai/cam"
# ---------------------------------------------

st.set_page_config(page_title="DyBro Dashboard", layout="wide")

# ---------------- SESSION STATE INIT ----------------
if "mqtt_data" not in st.session_state:
    st.session_state.mqtt_data = {
        "connected": False,
        "last_seen": None,
        "dht": {"temperature": None},
        "ir": {"isFokus": None, "noise": None},
        "button": {"state": None, "timeRemaining_s": None},
        "log": [],
    }

if "mqtt_thread" not in st.session_state:
    st.session_state.mqtt_thread = None
    st.session_state.mqtt_thread_stop = threading.Event()

mqtt_data = st.session_state.mqtt_data
lock = threading.Lock()
client = mqtt.Client()


# ---------------- MQTT CALLBACKS ----------------
def on_connect(client, userdata, flags, rc):
    now = datetime.utcnow().isoformat()
    with lock:
        mqtt_data["connected"] = (rc == 0)
        mqtt_data["last_seen"] = now
        msg = "CONNECTED ✅" if rc == 0 else f"CONNECT_FAILED rc={rc}"
        mqtt_data["log"].insert(0, (now, "-", msg))

    if rc == 0:
        client.subscribe([(TOPIC_DHT, 0), (TOPIC_IR, 0), (TOPIC_BUTTON, 0)])


def on_disconnect(client, userdata, rc):
    now = datetime.utcnow().isoformat()
    with lock:
        mqtt_data["connected"] = False
        mqtt_data["log"].insert(0, (now, "-", f"DISCONNECTED rc={rc}"))


def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode("utf-8", errors="ignore")
    now = datetime.utcnow().isoformat()

    try:
        parsed = json.loads(payload)
    except Exception:
        parsed = payload

    with lock:
        mqtt_data["last_seen"] = now
        mqtt_data["log"].insert(0, (now, topic, parsed))
        if len(mqtt_data["log"]) > 500:
            mqtt_data["log"] = mqtt_data["log"][:500]

        if topic == TOPIC_DHT and isinstance(parsed, dict):
            mqtt_data["dht"]["temperature"] = parsed.get("temperature")
        elif topic == TOPIC_IR and isinstance(parsed, dict):
            mqtt_data["ir"]["isFokus"] = parsed.get("isFokus")
            mqtt_data["ir"]["noise"] = parsed.get("noise")
        elif topic == TOPIC_BUTTON and isinstance(parsed, dict):
            mqtt_data["button"]["state"] = parsed.get("state")
            mqtt_data["button"]["timeRemaining_s"] = parsed.get("timeRemaining_s")


client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message


# ---------------- MQTT THREAD (AUTO RECONNECT) ----------------
def mqtt_loop(broker, port, stop_event):
    while not stop_event.is_set():
        try:
            if not mqtt_data["connected"]:
                client.connect(broker, port, keepalive=60)
                client.loop_start()
                time.sleep(1)
            time.sleep(0.1)
        except Exception as e:
            with lock:
                mqtt_data["log"].insert(
                    0, (datetime.utcnow().isoformat(), "CONNECT_ERROR", str(e))
                )
            time.sleep(5)  # retry after delay
    client.loop_stop()
    client.disconnect()


# ---------------- UI ----------------
st.title("DYBRO Dashboard")

with st.sidebar:
    st.header("Connection")

    broker = st.text_input("MQTT Broker", value=DEFAULT_BROKER)
    port = st.number_input("Port", value=DEFAULT_PORT, min_value=1, max_value=65535)

    if st.button("Connect"):
        if (
            st.session_state.mqtt_thread is None
            or not st.session_state.mqtt_thread.is_alive()
        ):
            st.session_state.mqtt_thread_stop.clear()
            thread = threading.Thread(
                target=mqtt_loop,
                args=(broker, int(port), st.session_state.mqtt_thread_stop),
                daemon=True,
            )
            st.session_state.mqtt_thread = thread
            thread.start()
            st.success("Connecting to MQTT broker...")
        else:
            st.info("Already connected or connecting...")

    if st.button("Disconnect"):
        st.session_state.mqtt_thread_stop.set()
        with lock:
            mqtt_data["connected"] = False
        st.info("Disconnected from broker.")

    st.markdown("---")
    st.write("Topics:")
    st.text(TOPIC_DHT)
    st.text(TOPIC_IR)
    st.text(TOPIC_BUTTON)
    st.markdown("---")
    st.caption("💡 Pastikan ESP32 & laptop sama-sama online dan broker aktif.")


# ---------------- Main Layout ----------------
col1, col2, col3 = st.columns(3)

with col1:
    st.subheader("Connection")
    with lock:
        connected = mqtt_data["connected"]
        last_seen = mqtt_data["last_seen"]
    st.metric("MQTT Connected", "✅ Yes" if connected else "❌ No")
    st.write("Last message:", last_seen if last_seen else "—")

with col2:
    st.subheader("Session")
    with lock:
        state = mqtt_data["button"].get("state")
        remaining = mqtt_data["button"].get("timeRemaining_s")
    st.metric("State", state if state else "—")
    if remaining is not None:
        try:
            remaining = int(remaining)
            mins = remaining // 60
            secs = remaining % 60
            st.metric("Remaining", f"{mins:02d}:{secs:02d}")
        except Exception:
            st.write("Remaining:", remaining)
    else:
        st.metric("Remaining", "—")

with col3:
    st.subheader("Sensors")
    with lock:
        temp = mqtt_data["dht"].get("temperature")
        isFokus = mqtt_data["ir"].get("isFokus")
        noise = mqtt_data["ir"].get("noise")
    st.metric("Temperature (°C)", f"{temp:.2f}" if temp is not None else "—")
    st.metric("Focus (isFokus)", str(isFokus) if isFokus is not None else "—")
    st.metric("Noise (stddev)", f"{noise:.2f}" if noise is not None else "—")


# ---------------- Log Table ----------------
st.markdown("---")
st.subheader("Live MQTT Log (most recent first)")
log_area = st.empty()
refresh_sec = st.sidebar.slider("Refresh interval (s)", min_value=1, max_value=5, value=2)


def render_log():
    with lock:
        entries = list(mqtt_data["log"])[:200]
    if not entries:
        log_area.write("No messages yet.")
        return
    rows = []
    for e in entries:
        if len(e) == 2:
            ts, msg = e
            rows.append({"time": ts, "topic": "-", "payload": str(msg)})
        else:
            ts, topic, payload = e
            payload_str = (
                json.dumps(payload, ensure_ascii=False)
                if not isinstance(payload, str)
                else payload
            )
            rows.append({"time": ts, "topic": topic, "payload": payload_str})
    log_area.table(rows)


# Auto refresh dashboard
st_autorefresh(interval=refresh_sec * 1000, limit=None, key="refresh_key")
render_log()
