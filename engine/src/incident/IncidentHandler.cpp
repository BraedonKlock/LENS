#include "IncidentHandler.h"

#include <curl/curl.h>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <thread>

static size_t writeCallback(char* ptr, size_t size, size_t nmemb, std::string* out)
{
	out->append(ptr, size * nmemb);
	return size * nmemb;
}

static std::string buildJson(std::initializer_list<std::pair<std::string, std::string>> fields)
{
	std::string json = "{";
	bool first = true;
	for (auto& [k, v] : fields) {
		if (!first) json += ",";
		json += "\"" + k + "\":\"" + v + "\"";
		first = false;
	}
	return json + "}";
}

IncidentHandler::IncidentHandler(const std::string& backendUrl, const std::string& apiKey)
	: m_backendUrl(backendUrl), m_apiKey(apiKey)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

void IncidentHandler::triggerShared(std::shared_ptr<IncidentHandler> self,
                                    const std::vector<cv::Mat>& frames, double fps,
                                    int cameraId, const std::string& incidentType)
{
	if (self->m_uploading.exchange(true)) return;
	std::thread([self, frames, fps, cameraId, incidentType]() {
		self->process(frames, fps, cameraId, incidentType);
		self->m_uploading = false;
	}).detach();
}

void IncidentHandler::trigger(const std::vector<cv::Mat>& frames, double fps,
                               int cameraId, const std::string& incidentType)
{
	if (m_uploading.exchange(true)) return;

	std::thread([this, frames, fps, cameraId, incidentType]() {
		process(frames, fps, cameraId, incidentType);
		m_uploading = false;
	}).detach();
}

void IncidentHandler::process(std::vector<cv::Mat> frames, double fps,
                               int cameraId, std::string incidentType)
{
	auto now = std::chrono::system_clock::now();
	auto t   = std::chrono::system_clock::to_time_t(now);
	std::ostringstream ss;
	ss << std::put_time(std::gmtime(&t), "%Y%m%dT%H%M%SZ");
	std::string timestamp = ss.str();

	std::string clipFile = recordClip(frames, fps, timestamp);
	if (clipFile.empty()) return;

	std::string uploadUrl, r2ClipPath;
	if (!getPresignedUrl(cameraId, timestamp, uploadUrl, r2ClipPath)) {
		std::filesystem::remove(clipFile);
		return;
	}

	if (!uploadClip(clipFile, uploadUrl)) {
		std::filesystem::remove(clipFile);
		return;
	}

	std::filesystem::remove(clipFile);
	reportIncident(cameraId, timestamp, r2ClipPath, incidentType);
}

std::string IncidentHandler::recordClip(const std::vector<cv::Mat>& frames, double fps,
                                         const std::string& timestamp)
{
	if (frames.empty()) return {};

	std::string path = "/tmp/lens_" + timestamp + ".mp4";
	cv::Size size(frames[0].cols, frames[0].rows);
	cv::VideoWriter writer(path, cv::VideoWriter::fourcc('m','p','4','v'), fps, size);

	if (!writer.isOpened()) return {};

	for (const auto& f : frames)
		writer.write(f);

	return path;
}

bool IncidentHandler::getPresignedUrl(int cameraId, const std::string& timestamp,
                                       std::string& outUploadUrl, std::string& outClipPath)
{
	CURL* curl = curl_easy_init();
	if (!curl) return false;

	std::string body = buildJson({
		{"cameraId", std::to_string(cameraId)},
		{"timestamp", timestamp}
	});
	std::string response;

	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, ("X-API-Key: " + m_apiKey).c_str());
	headers = curl_slist_append(headers, "Content-Type: application/json");

	std::string url = m_backendUrl + "/engine/incidents/request-upload";
	curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);

	CURLcode res = curl_easy_perform(curl);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) return false;

	auto extract = [&](const std::string& key) -> std::string {
		std::string search = "\"" + key + "\":\"";
		auto pos = response.find(search);
		if (pos == std::string::npos) return {};
		pos += search.size();
		auto end = response.find('"', pos);
		return end != std::string::npos ? response.substr(pos, end - pos) : "";
	};

	outUploadUrl = extract("uploadUrl");
	outClipPath  = extract("clipPath");
	return !outUploadUrl.empty() && !outClipPath.empty();
}

bool IncidentHandler::uploadClip(const std::string& filePath, const std::string& uploadUrl)
{
	FILE* f = fopen(filePath.c_str(), "rb");
	if (!f) return false;

	fseek(f, 0, SEEK_END);
	long fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	CURL* curl = curl_easy_init();
	if (!curl) { fclose(f); return false; }

	curl_easy_setopt(curl, CURLOPT_URL,                uploadUrl.c_str());
	curl_easy_setopt(curl, CURLOPT_UPLOAD,             1L);
	curl_easy_setopt(curl, CURLOPT_READDATA,           f);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,   (curl_off_t)fileSize);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(f);

	return res == CURLE_OK;
}

bool IncidentHandler::reportIncident(int cameraId, const std::string& timestamp,
                                      const std::string& clipPath, const std::string& incidentType)
{
	CURL* curl = curl_easy_init();
	if (!curl) return false;

	std::string body = buildJson({
		{"cameraId",     std::to_string(cameraId)},
		{"timestamp",    timestamp},
		{"clipPath",     clipPath},
		{"incidentType", incidentType}
	});
	std::string response;

	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, ("X-API-Key: " + m_apiKey).c_str());
	headers = curl_slist_append(headers, "Content-Type: application/json");

	std::string url = m_backendUrl + "/engine/incidents";
	curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    body.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);

	CURLcode res = curl_easy_perform(curl);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	return res == CURLE_OK;
}
