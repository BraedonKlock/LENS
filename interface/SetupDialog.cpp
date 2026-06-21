#include "SetupDialog.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>

static constexpr const char* ENGINE_BASE  = "http://127.0.0.1:7373";
static constexpr const char* BACKEND_URL  = "http://192.168.1.234:3000";

// ── helpers ──────────────────────────────────────────────────────────────────

static QLabel* makeLabel(QWidget* parent, const QString& text)
{
	auto* l = new QLabel(text, parent);
	l->setStyleSheet(
		"font-size: 11px; font-weight: 600; color: #4a6a8a;"
		" letter-spacing: 0.6px; text-transform: uppercase;"
		" background: transparent;");
	return l;
}

static QLineEdit* makeField(QWidget* parent, const QString& placeholder, bool password = false)
{
	auto* f = new QLineEdit(parent);
	f->setPlaceholderText(placeholder);
	f->setFixedHeight(42);
	f->setStyleSheet(
		"QLineEdit {"
		"  background: #0c1525;"
		"  border: 1px solid #1a2d4a;"
		"  border-radius: 8px;"
		"  color: #d4e4f7;"
		"  font-size: 13px;"
		"  padding: 0 14px;"
		"}"
		"QLineEdit:focus { border-color: #2563eb; }"
		"QLineEdit::placeholder { color: #2a4060; }");
	if (password) f->setEchoMode(QLineEdit::Password);
	return f;
}

// ── SetupDialog ───────────────────────────────────────────────────────────────

SetupDialog::SetupDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle("LENS — Create Account");
	setFixedWidth(480);
	setModal(true);
	setStyleSheet("background-color: #07090f;");

	m_nam = new QNetworkAccessManager(this);

	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(36, 32, 36, 32);
	root->setSpacing(16);

	// ── Logo + heading ────────────────────────────────────────────────────
	auto* logoLabel = new QLabel(this);
	QPixmap logo(":/logo/LENS_LOGO.jpg");
	logoLabel->setPixmap(logo.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	logoLabel->setAlignment(Qt::AlignHCenter);
	root->addWidget(logoLabel);

	auto* title = new QLabel("Create Account", this);
	title->setAlignment(Qt::AlignHCenter);
	title->setStyleSheet("font-size: 18px; font-weight: 700; color: #c8d6ef; background: transparent;");
	root->addWidget(title);

	auto* subtitle = new QLabel("Enter your server ID and store details.", this);
	subtitle->setAlignment(Qt::AlignHCenter);
	subtitle->setStyleSheet("font-size: 12px; color: #2a4060; background: transparent;");
	root->addWidget(subtitle);

	// ── Status label ─────────────────────────────────────────────────────
	m_statusLabel = new QLabel("", this);
	m_statusLabel->setAlignment(Qt::AlignHCenter);
	m_statusLabel->setWordWrap(true);
	m_statusLabel->setFixedHeight(18);
	m_statusLabel->setStyleSheet("font-size: 12px; color: #ef4444; background: transparent;");
	root->addWidget(m_statusLabel);

	// ── Form ─────────────────────────────────────────────────────────────
	auto* form = new QVBoxLayout();
	form->setSpacing(6);

	auto addRow = [&](const QString& labelText, QLineEdit* field) {
		form->addWidget(makeLabel(this, labelText));
		form->addWidget(field);
		form->addSpacing(6);
	};

	m_serverId    = makeField(this, "LENS-XXXX-XXXX-XXXX");
	m_email       = makeField(this, "manager@store.com");
	m_password    = makeField(this, "Password", true);
	m_storeName   = makeField(this, "Downtown Location");
	m_storeNumber = makeField(this, "1042");

	m_email->setInputMethodHints(Qt::ImhEmailCharactersOnly);

	addRow("Server ID",    m_serverId);
	addRow("Email",        m_email);
	addRow("Password",     m_password);
	addRow("Store Name",   m_storeName);
	addRow("Store Number", m_storeNumber);

	root->addLayout(form);

	// ── Submit button ─────────────────────────────────────────────────────
	m_submitBtn = new QPushButton("Create Account", this);
	m_submitBtn->setFixedHeight(46);
	m_submitBtn->setStyleSheet(
		"QPushButton {"
		"  background: #2563eb; color: white; border: none;"
		"  border-radius: 10px; font-size: 14px; font-weight: 700;"
		"}"
		"QPushButton:hover    { background: #3b82f6; }"
		"QPushButton:pressed  { background: #1d4ed8; }"
		"QPushButton:disabled { background: #1a2d4a; color: #3a5a7a; }");
	connect(m_submitBtn, &QPushButton::clicked, this, &SetupDialog::onSubmit);
	root->addWidget(m_submitBtn);

	adjustSize();
}

// ── status helpers ────────────────────────────────────────────────────────────

void SetupDialog::setStatus(const QString& msg, bool isError)
{
	m_statusLabel->setText(msg);
	m_statusLabel->setStyleSheet(
		isError ? "font-size: 12px; color: #ef4444; background: transparent;"
		        : "font-size: 12px; color: #22c55e; background: transparent;");
}

void SetupDialog::setLoading(bool loading, const QString& msg)
{
	m_submitBtn->setEnabled(!loading);
	m_submitBtn->setText(loading ? msg : "Create Account");
}

// ── actions ───────────────────────────────────────────────────────────────────

void SetupDialog::onSubmit()
{
	if (m_serverId->text().trimmed().isEmpty()   ||
	    m_email->text().trimmed().isEmpty()       ||
	    m_password->text().isEmpty()              ||
	    m_storeName->text().trimmed().isEmpty()   ||
	    m_storeNumber->text().trimmed().isEmpty())
	{
		setStatus("All fields are required.", true);
		return;
	}

	setStatus("");
	setLoading(true, "Registering...");
	doRegister();
}

void SetupDialog::doRegister()
{
	QNetworkRequest req(QUrl(QString("%1/auth/register").arg(BACKEND_URL)));
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QJsonObject body;
	body["server_id"]    = m_serverId->text().trimmed();
	body["email"]        = m_email->text().trimmed();
	body["password"]     = m_password->text();
	body["store_name"]   = m_storeName->text().trimmed();
	body["store_number"] = m_storeNumber->text().trimmed();

	auto* reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();

		if (reply->error() != QNetworkReply::NoError) {
			setLoading(false);
			setStatus("Could not reach backend: " + reply->errorString(), true);
			return;
		}

		const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
		if (obj.contains("error")) {
			setLoading(false);
			setStatus(obj["error"].toString(), true);
			return;
		}

		const QString key = obj["engine_api_key"].toString();
		if (key.isEmpty()) {
			setLoading(false);
			setStatus("Unexpected response from server.", true);
			return;
		}

		setLoading(true, "Configuring engine...");
		doEngineSetup(key);
	});
}

void SetupDialog::doEngineSetup(const QString& apiKey)
{
	QNetworkRequest req(QUrl(QString("%1/config/setup").arg(ENGINE_BASE)));
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QJsonObject body;
	body["api_key"]     = apiKey;
	body["backend_url"] = QString(BACKEND_URL);

	auto* reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		setLoading(false);

		if (reply->error() != QNetworkReply::NoError) {
			setStatus("Account created but engine config failed:\n" + reply->errorString(), true);
			return;
		}

		setStatus("Account created successfully!", false);
		QTimer::singleShot(900, this, [this]() { accept(); });
	});
}
