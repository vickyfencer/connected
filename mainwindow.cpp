#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QDebug>

MainWindow *handle;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->count = 50;
    this->time = 0;
    this->setWindowTitle("EE513 Assignment 2");
    this->ui->customPlot->addGraph();
    this->ui->customPlot->yAxis->setLabel("Pitch (degrees)");
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    this->ui->customPlot->xAxis->setTicker(timeTicker);
    this->ui->customPlot->yAxis->setRange(-180,180);
    this->ui->customPlot->replot();
    QObject::connect(this, SIGNAL(messageSignal(QString)),
                     this, SLOT(on_MQTTmessage(QString)));
    ::handle = this;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::update(){
    // For more help on real-time plots, see: http://www.qcustomplot.com/index.php/demos/realtimedatademo
    static QTime time(QTime::currentTime());
    double key = time.elapsed()/1000.0; // time elapsed since start of demo, in seconds
    ui->customPlot->graph(0)->addData(key,count);
    ui->customPlot->graph(0)->rescaleKeyAxis(true);
    ui->customPlot->replot();
    QString text = QString("Value added is %1").arg(this->count);
    ui->outputEdit->setText(text);
}

void MainWindow::on_downButton_clicked()
{
    this->count-=10;
    this->update();
}

void MainWindow::on_upButton_clicked()
{
    this->count+=10;
    this->update();
}

void MainWindow::on_rollButton_clicked()
{
    QString message = "roll button clicked";
    ui->outputText->appendPlainText(message);
    this->update();
}
void MainWindow::on_pitchButton_clicked()
{
    QString message = "pitch button clicked";
    ui->outputText->appendPlainText(message);
    this->update();
}
void MainWindow::on_cpuButton_clicked()
{
    QString message = "cpu button clicked";
    ui->outputText->appendPlainText(message);
    this->update();
}

void MainWindow::on_connectButton_clicked()
{
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    if (MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)==0){
        ui->outputText->appendPlainText(QString("Callbacks set correctly"));
    }
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        ui->outputText->appendPlainText(QString("Failed to connect, return code %1").arg(rc));
    }
    ui->outputText->appendPlainText(QString("Subscribing to topic " TOPIC " for client " CLIENTID));
    int x = MQTTClient_subscribe(client, TOPIC, QOS);
    ui->outputText->appendPlainText(QString("Result of subscribe is %1 (0=success)").arg(x));
}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    (void)context;
    // Please don't modify the Window UI from here
    qDebug() << "Message delivery confirmed";
    handle->deliveredtoken = dt;
}

/* This is a callback function and is essentially another thread. Do not modify the
 * main window UI from here as it will cause problems. Please see the Slot method that
 * is directly below this function. To ensure that this method is thread safe I had to
 * get it to emit a signal which is received by the slot method below */
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    (void)context; (void)topicLen;
    qDebug() << "Message arrived (topic is " << topicName << ")";
    qDebug() << "Message payload length is " << message->payloadlen;
    QString payload;
    payload.sprintf("%s", (char *) message->payload).truncate(message->payloadlen);
    emit handle->messageSignal(payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

/** This is the slot method. Do all of your message received work here. It is also safe
 * to call other methods on the object from this point in the code */
void MainWindow::on_MQTTmessage(QString payload){
    ui->outputText->appendPlainText(payload);
    ui->outputText->ensureCursorVisible();
    // Remove the extra } at the end of the JSON string
       if (payload.endsWith("}")) {
           payload.chop(1);
       }

    parseJSONData(payload);

    //ADD YOUR CODE HERE
}

void connlost(void *context, char *cause) {
    (void)context; (void)*cause;
    // Please don't modify the Window UI from here
    qDebug() << "Connection Lost" << endl;
}

int MainWindow::parseJSONData(const QString &str) {
    qDebug() << "Received JSON string:" << str;

    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());
    if (doc.isNull()) {
        qDebug() << "Failed to parse JSON document.";
        return -1; // Return -1 to indicate failure
    }

    QJsonObject obj = doc.object();
    QStringList requiredKeys = {"CPUTemp", "pitch", "roll"};

    // Check if all required keys are present
    for (const QString &key : requiredKeys) {
        if (!obj.contains(key) || !obj[key].isDouble()) {
            qDebug() << "Invalid or missing data for key:" << key;
            return -1; // Return -1 to indicate failure
        }
    }

    // Extract and assign temperature
    double tempValue = obj["CPUTemp"].toDouble();
    if (qIsFinite(tempValue)) {
        temperature = static_cast<float>(tempValue);
        qDebug() << "Temperature extracted successfully:" << temperature;
    } else {
        qDebug() << "Invalid or missing temperature data.";
    }

    // Extract and assign pitch
    double pitchValue = obj["pitch"].toDouble();
    if (qIsFinite(pitchValue)) {
        pitch = static_cast<float>(pitchValue);
        qDebug() << "Pitch extracted successfully:" << pitch;
    } else {
        qDebug() << "Invalid or missing pitch data.";
    }

    // Extract and assign roll
    double rollValue = obj["roll"].toDouble();
    if (qIsFinite(rollValue)) {
        roll = static_cast<float>(rollValue);
        qDebug() << "Roll extracted successfully:" << roll;
    } else {
        qDebug() << "Invalid or missing roll data.";
    }

    return 0; // Return 0 to indicate success
}

void MainWindow::on_disconnectButton_clicked()
{
    qDebug() << "Disconnecting from the broker" << endl;
    MQTTClient_disconnect(client, 10000);
    //MQTTClient_destroy(&client);
}