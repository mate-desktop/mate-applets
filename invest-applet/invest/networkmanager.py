import mate_invest
from gi.repository import Gio

class NetworkManager:
	def __init__(self):
		self.network_available = True
		self.statechange_callback = None

		self.monitor = Gio.NetworkMonitor.get_default()
		self.monitor.connect('network-changed', self.on_network_changed)

	def online(self):
		return self.network_available

	def offline(self):
		return not self.online()

	# the signal handler for signals from the network manager
	def on_network_changed(self, monitor, available):
		self.network_available = available
		# notify about state change
		if self.statechange_callback != None:
			self.statechange_callback()

	def set_statechange_callback(self,handler):
		self.statechange_callback = handler
