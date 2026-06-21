#include "HttpServer.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

static constexpr const char* HOST = "127.0.0.1";
static constexpr int PORT = 7373;

// Resolves a path relative to this executable's directory.
// Falls back to path as-is if resolution fails.
static std::string resolveEnginePath(const std::string& relative)
{
	try {
		auto exeDir = std::filesystem::canonical("/proc/self/exe").parent_path();
		auto candidate = exeDir.parent_path() / relative; // exe is in build/, go up one
		if (std::filesystem::exists(candidate)) return candidate.string();
	} catch (...) {}
	return relative;
}

static std::string extractField(const std::string& json, const std::string& key)
{
	std::string search = "\"" + key + "\":\"";
	auto pos = json.find(search);
	if (pos == std::string::npos) return {};
	pos += search.size();
	auto end = json.find('"', pos);
	return end != std::string::npos ? json.substr(pos, end - pos) : "";
}

HttpServer::HttpServer(CameraManager& manager, CameraStore& store)
	: s_cameraManager(manager), s_cameraStore(store)
{
	registerRoutes();
}

void HttpServer::registerRoutes()
{
	// ── Config ────────────────────────────────────────────────────────────────

	s_server.Get("/config/status", [this](const httplib::Request&, httplib::Response& res)
	{
		const std::string apiKey = s_cameraStore.getConfig("api_key");
		const char* configured   = apiKey.empty() ? "false" : "true";
		res.set_content(std::string("{\"configured\":") + configured + "}", "application/json");
	});

	s_server.Post("/config/setup", [this](const httplib::Request& req, httplib::Response& res)
	{
		const std::string apiKey     = extractField(req.body, "api_key");
		const std::string backendUrl = extractField(req.body, "backend_url");

		if (apiKey.empty() || backendUrl.empty()) {
			res.status = 400;
			res.set_content(R"({"error":"api_key and backend_url required"})", "application/json");
			return;
		}

		s_cameraStore.setConfig("api_key",     apiKey);
		s_cameraStore.setConfig("backend_url", backendUrl);
		s_cameraManager.setConfig(backendUrl, apiKey);

		res.set_content(R"({"success":true})", "application/json");
	});

	// ── Test trigger ─────────────────────────────────────────────────────────

	s_server.Post("/test/trigger", [this](const httplib::Request& req, httplib::Response& res)
	{
		const std::string apiKey     = s_cameraStore.getConfig("api_key");
		const std::string backendUrl = s_cameraStore.getConfig("backend_url");

		if (apiKey.empty() || backendUrl.empty()) {
			res.status = 400;
			res.set_content(R"({"error":"engine not configured — create an account first"})",
			                "application/json");
			return;
		}

		if (s_testHandler && s_testHandler->isUploading()) {
			res.status = 409;
			res.set_content(R"({"error":"previous test still uploading"})", "application/json");
			return;
		}

		int camId = 1;
		std::string camIdStr = extractField(req.body, "camera_id");
		if (!camIdStr.empty()) { try { camId = std::stoi(camIdStr); } catch (...) {} }

		// Generate a 5-second test clip at 30fps
		constexpr int    FPS    = 30;
		constexpr int    FRAMES = FPS * 5;
		constexpr int    W      = 640;
		constexpr int    H      = 480;

		std::vector<cv::Mat> frames;
		frames.reserve(FRAMES);
		for (int i = 0; i < FRAMES; ++i) {
			cv::Mat f(H, W, CV_8UC3, cv::Scalar(10, 10, 20));
			cv::putText(f, "LENS TEST INCIDENT",
			            {60, 180}, cv::FONT_HERSHEY_DUPLEX, 1.3, {0, 210, 100}, 2);
			cv::putText(f, "Camera " + std::to_string(camId) + "  |  Frame " + std::to_string(i),
			            {60, 250}, cv::FONT_HERSHEY_SIMPLEX, 0.8, {100, 160, 255}, 1);
			cv::putText(f, "Concealment suspected",
			            {60, 320}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {60, 120, 255}, 1);
			frames.push_back(f);
		}

		s_testHandler = std::make_shared<IncidentHandler>(backendUrl, apiKey);
		IncidentHandler::triggerShared(s_testHandler, frames, FPS, camId, "test");

		std::cerr << "[test] incident triggered for camera " << camId << "\n";
		res.set_content(R"({"success":true,"message":"test incident triggered — check your mobile"})",
		                "application/json");
	});

	// ── Video file test (runs actual AI inference on a .mp4) ─────────────────

	s_server.Post("/test/video", [this](const httplib::Request& req, httplib::Response& res)
	{
		const std::string apiKey     = s_cameraStore.getConfig("api_key");
		const std::string backendUrl = s_cameraStore.getConfig("backend_url");

		if (apiKey.empty() || backendUrl.empty()) {
			res.status = 400;
			res.set_content(R"({"error":"engine not configured — create an account first"})",
			                "application/json");
			return;
		}

		const std::string videoPath = extractField(req.body, "path");
		if (videoPath.empty()) {
			res.status = 400;
			res.set_content(R"({"error":"path required"})", "application/json");
			return;
		}

		int camId = 1;
		std::string camIdStr = extractField(req.body, "camera_id");
		if (!camIdStr.empty()) { try { camId = std::stoi(camIdStr); } catch (...) {} }

		if (s_testHandler && s_testHandler->isUploading()) {
			res.status = 409;
			res.set_content(R"({"error":"previous test still uploading"})", "application/json");
			return;
		}

		cv::VideoCapture cap(videoPath);
		if (!cap.isOpened()) {
			res.status = 400;
			res.set_content(R"({"error":"could not open video file"})", "application/json");
			return;
		}

		double fps = cap.get(cv::CAP_PROP_FPS);
		if (fps <= 0) fps = 30.0;

#ifdef LENS_INFERENCE_ENABLED
		Inferencer inferencer(resolveEnginePath("models/model.onnx"));
		if (!inferencer.load()) {
			res.status = 500;
			res.set_content(R"({"error":"failed to load model"})", "application/json");
			return;
		}

		// Read all frames, then classify the full clip
		std::vector<cv::Mat> allFrames;
		cv::Mat frame;
		while (cap.read(frame)) allFrames.push_back(frame.clone());
		cap.release();

		if (allFrames.empty()) {
			res.status = 400;
			res.set_content(R"({"error":"video file is empty"})", "application/json");
			return;
		}

		std::cerr << "[video] running inference on " << allFrames.size() << " frames...\n";
		if (!inferencer.classify(allFrames)) {
			res.set_content(
				R"({"success":false,"message":"model did not flag this clip as suspicious"})",
				"application/json");
			return;
		}

		std::vector<cv::Mat> clip = std::move(allFrames);
#else
		// Without inference: treat the whole video as the incident clip
		std::vector<cv::Mat> clip;
		cv::Mat frame;
		while (cap.read(frame)) clip.push_back(frame.clone());
		cap.release();

		if (clip.empty()) {
			res.status = 400;
			res.set_content(R"({"error":"video file is empty"})", "application/json");
			return;
		}
#endif

		s_testHandler = std::make_shared<IncidentHandler>(backendUrl, apiKey);
		IncidentHandler::triggerShared(s_testHandler, clip, fps, camId, "concealment_detected");

		std::cerr << "[video] incident triggered from file: " << videoPath << "\n";
		res.set_content(R"({"success":true,"message":"video processed — check your mobile"})",
		                "application/json");
	});

	// ── Cameras ───────────────────────────────────────────────────────────────

	s_server.Get("/cameras", [this](const httplib::Request&, httplib::Response& res)
	{
		auto cameras = s_cameraStore.loadAll();
		std::string json = "[";
		for (size_t i = 0; i < cameras.size(); ++i) {
			const auto& c   = cameras[i];
			const bool  con = s_cameraManager.isConnected(c.id);
			if (i > 0) json += ",";
			json += "{";
			json += "\"id\":"          + std::to_string(c.id)          + ",";
			json += "\"name\":\""      + c.name                        + "\",";
			json += "\"url\":\""       + c.url                         + "\",";
			json += "\"location\":\""  + c.location                    + "\",";
			json += std::string("\"connected\":") + (con ? "true" : "false");
			json += "}";
		}
		json += "]";
		res.set_content(json, "application/json");
	});

	s_server.Post("/cameras", [this](const httplib::Request& req, httplib::Response& res)
	{
		CameraConfig config;
		config.name     = extractField(req.body, "name");
		config.url      = extractField(req.body, "url");
		config.password = extractField(req.body, "password");
		config.location = extractField(req.body, "location");

		if (config.name.empty() || config.url.empty()) {
			res.status = 400;
			res.set_content(R"({"error":"name and url required"})", "application/json");
			return;
		}

		config.id = s_cameraStore.save(config);
		if (config.id < 0) {
			res.status = 500;
			res.set_content(R"({"error":"failed to save camera"})", "application/json");
			return;
		}

		s_cameraManager.addCamera(config);

		res.status = 201;
		res.set_content("{\"id\":" + std::to_string(config.id) + "}", "application/json");
	});

	s_server.Delete("/cameras/:id", [this](const httplib::Request& req, httplib::Response& res)
	{
		int id = std::stoi(req.path_params.at("id"));
		s_cameraStore.remove(id);
		s_cameraManager.removeCamera(id);
		res.status = 204;
	});
}

void HttpServer::start()
{
	s_thread = std::thread([this]()
	{
		s_server.listen(HOST, PORT);
	});
}

void HttpServer::stop()
{
	s_server.stop();
	if (s_thread.joinable()) s_thread.join();
}
