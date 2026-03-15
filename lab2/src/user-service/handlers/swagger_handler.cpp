#include "swagger_handler.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace profi::handlers {

namespace {

const std::string kSwaggerHtml = R"html(<!DOCTYPE html>
<html>
<head>
  <title>Profi Service API</title>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css">
</head>
<body>
<div id="swagger-ui"></div>
<script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js"></script>
<script>
window.onload = function() {
  SwaggerUIBundle({
    url: "/openapi.yaml",
    dom_id: '#swagger-ui',
    presets: [SwaggerUIBundle.presets.apis, SwaggerUIBundle.SwaggerUIStandalonePreset],
    layout: "BaseLayout",
    tryItOutEnabled: true
  });
};
</script>
</body>
</html>
)html";

}  // namespace

std::string SwaggerUiHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType("text/html; charset=utf-8");
    return kSwaggerHtml;
}

OpenApiHandler::OpenApiHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {
    std::ifstream f("openapi.yaml");
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open openapi.yaml");
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    spec_ = ss.str();
}

std::string OpenApiHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType("application/yaml");
    return spec_;
}

}  // namespace profi::handlers
