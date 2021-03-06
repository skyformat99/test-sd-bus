#include "serviced-a.h"

Manager *manager = NULL;

static void manager_free(Manager *manager);

static void manager_reset_config(Manager *manager) {
	// Do some relative configurations here.

}

static Manager *manager_new(void) {
	Manager *manager;

	manager = new0(Manager, 1);
	if (!manager)
		return NULL;

	manager_reset_config(manager);

	return manager;
}

static void manager_free(Manager *manager) {

	printf("Free sd-bus\n");

	sd_bus_slot_unref(manager->slot);
	sd_bus_unref(manager->bus);

	free(manager);
}

static int manager_connect_bus(Manager *manager) {
	int r;
	sd_id128_t id;
	
	const char *unique;

	/* Connect to the user bus this time */
	r = sd_bus_open_user(&manager->bus);
	if(r < 0) {
		fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
		return r;
	}

	r = sd_bus_get_bus_id(manager->bus, &id);
	if(r < 0) {
		fprintf(stderr, "Failed to get server ID: %s\n", strerror(-r));
		return r;
	}

	r = sd_bus_get_unique_name(manager->bus, &unique);
	if(r < 0) {
		fprintf(stderr, "Failed to get unique name: %s\n", strerror(-r));
		return r;
	}

	printf("Peer ID is " SD_ID128_FORMAT_STR ".\n", SD_ID128_FORMAT_VAL(id));
	printf("Unique ID: %s\n", unique);

	/* Install the object */
	r = sd_bus_add_object_vtable(
		manager->bus,
		&manager->slot,
		OBJECT_PATH,  /* object path */
		INTERFACE_NAME,   /* interface name */
		service_vtable,
		NULL);
	if(r < 0) {
		fprintf(stderr, "Failed to install object: %s\n", strerror(-r));
		return r;
	}

	/* Take a well-known service name so that clients can find us */
	r = sd_bus_request_name(manager->bus, SERVICE_NAME, 0);
	if(r < 0) {
		fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
		return r;
	}

	// This is match_function for testing, so mark this
	/*r = sd_bus_add_match(
		manager->bus, 
		NULL,
		"type='signal',"
		"sender='org.freedesktop.DBus',"
		"interface='org.freedesktop.DBus',"
		"member='NameOwnerChanged',"
		"path='/org/freedesktop/DBus'",
		match_name_owner_changed,
		NULL);

	if(r < 0) {
		fprintf(stderr, "Failed to add match: %s\n", strerror(-r));
		goto finish;
	}*/

	r = sd_bus_add_match(
		manager->bus, 
		NULL,
		"type='signal',"
		"sender='com.cybernut.demoB',"
		"interface='com.cybernut.demoB.ServiceB',"
		"member='SignalTestB',"
		"path='/com/cybernut/demoB/ServiceB'",
		match_signal_test,
		NULL);

	if(r < 0) {
		fprintf(stderr, "Failed to add match: %s\n", strerror(-r));
		return r;
	}

	return 0;
}

static int manager_startup(Manager *manager) {
	int r;

	/* Connect to the bus */
	r = manager_connect_bus(manager);
	if (r < 0)
		return r;

	return 0;
}

static int manager_run(Manager *manager) {
	int r;

	for(;;) {
		/* Process requests */
		r = sd_bus_process(manager->bus, NULL);
		if (r < 0) {
			fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
			return r;
		}
		if (r > 0) /* we processed a request, try to process another one, right-away */
			continue;

		/* Wait for the next request to process */
		r = sd_bus_wait(manager->bus, (uint64_t) -1);
		if (r < 0) {
			fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
			return r;
		}
	}

	
}

int main(int argc, char *argv[]) {
	int r;

	manager = manager_new();
	if (!manager) {
		fprintf(stderr, "Failed to create manager.\n");
		goto finish;
	}

	r = manager_startup(manager);
	if (r < 0) {
		fprintf(stderr, "Failed to fully start up daemon: %s\n", strerror(-r));
		goto finish;
	}

	printf("serviced-a is running...\n");

	r = manager_run(manager);

finish:
	manager_free(manager);
	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}