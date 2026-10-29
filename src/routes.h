#pragma once
#include <httplib.h>

#include "service.h"

// Регистрирует в HTTP-сервере все 15 маршрутов API
void register_routes(httplib::Server& server, ShopService& service);
