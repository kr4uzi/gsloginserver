#include <boost/asio.hpp>
#include "memorydb.h"
#include "bf2available.h"
#include "protocol.h"
#include "gpcm.h"
#include "gpsp.h"

int main() {
	boost::asio::io_service service;

	auto db = gamespy::database::MemoryDB();
	auto bf2available = gamespy::available::battlefield2::Server(service);
	auto gpcm = gamespy::gpcm::Server(service, db);
	auto gpsp = gamespy::gpsp::Server(service, db);

	gpcm.start();
	gpsp.start();

	service.run();
}