import mate_invest
from mate_invest.defs import NETWORKMANAGER_VERSION
from dbus.mainloop.glib import DBusGMainLoop
import dbus

# possible states, see http://projects.gnome.org/NetworkManager/developers/ -> spec 0.8 -> NM_STATE
STATE_UNKNOWN		= dbus.UInt32(0)
STATE_ASLEEP		= dbus.UInt32(1)
STATE_CONNECTING	= dbus.UInt32(2)
STATE_CONNECTED		= dbus.UInt32(3)
STATE_DISCONNEDTED	= dbus.UInt32(4)

# numerical values of these states depend on the network manager version, they changed with 0.8.995
fields = NETWORKMANAGER_VERSION.split('.')
if len(fields) >= 2:
    major = int(fields[0])
    minor = int(fields[1])
    if len(fields) > 2:
        micro = int(fields[2])

    if major > 0 or major == 0 and (minor >= 9 or len(fields) > 2 and minor == 8 and micro >= 995):
        # see http://projects.gnome.org/NetworkManager/developers/ -> spec 0.9 -> NM_STATE
        print("Found NetworkManager spec 0.9 (%s)" % NETWORKMANAGER_VERSION)
        STATE_UNKNOWN = dbus.UInt32(0)
        STATE_ASLEEP = dbus.UInt32(10)
        STATE_DISCONNECTED = dbus.UInt32(20)
        STATE_DISCONNECTING = dbus.UInt32(30)
        STATE_CONNECTING = dbus.UInt32(40)
        STATE_CONNECTED_LOCAL = dbus.UInt32(50)
        STATE_CONNECTED_SITE = dbus.UInt32(60)
        STATE_CONNECTED_GLOBAL = dbus.UInt32(70)
        STATE_CONNECTED = STATE_CONNECTED_GLOBAL # backward compatibility with < 0.9

class NetworkManager:
	def __init__(self):
		self.state = STATE_UNKNOWN
		self.statechange_callback = None

		try:
			# get an event loop
			loop = DBusGMainLoop()

			# get the NetworkManager object from D-Bus
			mate_invest.debug("Connecting to Network Manager via D-Bus")
			bus = dbus.SystemBus(mainloop=loop)
			nmobj = bus.get_object('org.freedesktop.NetworkManager', '/org/freedesktop/NetworkManager')
			nm = dbus.Interface(nmobj, 'org.freedesktop.NetworkManager')

			# connect the signal handler to the bus
			bus.add_signal_receiver(self.handler, None,
					'org.freedesktop.NetworkManager',
					'org.freedesktop.NetworkManager',
					'/org/freedesktop/NetworkManager')

			# get the current status of the network manager
			self.state = nm.state()
			mate_invest.debug("Current Network Manager status is %d" % self.state)
		except Exception, msg:
			mate_invest.error("Could not connect to the Network Manager: %s" % msg )

	def online(self):
		return self.state == STATE_UNKNOWN or self.state == STATE_CONNECTED

	def offline(self):
		return not self.online()

	# the signal handler for signals from the network manager
	def handler(self,signal=None):
		if isinstance(signal, dict):
			state = signal.get('State')
			if state != None:
				mate_invest.debug("Network Manager change state %d => %d" % (self.state, state) );
				self.state = state

				# notify about state change
				if self.statechange_callback != None:
					self.statechange_callback()

	def set_statechange_callback(self,handler):
		self.statechange_callback = handler
