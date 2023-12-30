// mqtt_manager.dart
import 'dart:io';
import 'package:flutter/services.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

class MQTTManager {
  late MqttServerClient client;
  String statusText = "Connected";
  bool isConnected = true;
  Function(String)? onNewMessage;

  MQTTManager({this.onNewMessage}) {
    initializeMQTTClient();
  }

  Future<void> initializeMQTTClient() async {
    client = MqttServerClient(
        'an726pjx0w8v9-ats.iot.eu-north-1.amazonaws.com', 'flutter_client')
      ..logging(on: true)
      ..port = 8883
      ..secure = true;

    try {
      final rootCA = await rootBundle.load('assets/certs/RootCA.pem');
      final deviceCert =
          await rootBundle.load('assets/certs/DeviceCertificate.crt');
      final privateKey = await rootBundle.load('assets/certs/Private.key');

      SecurityContext context = SecurityContext.defaultContext;
      context.setTrustedCertificatesBytes(rootCA.buffer.asUint8List());
      context.useCertificateChainBytes(deviceCert.buffer.asUint8List());
      context.usePrivateKeyBytes(privateKey.buffer.asUint8List());
      client.securityContext = context;
    } catch (e) {
      print('Unable to load certificates: $e');
      return;
    }

    final connMess = MqttConnectMessage()
        .withClientIdentifier('flutter_client')
        .keepAliveFor(60)
        .startClean()
        .withWillTopic('willtopic')
        .withWillMessage('My Will message')
        .withWillQos(MqttQos.atLeastOnce);
    client.connectionMessage = connMess;

    try {
      await client.connect();
    } catch (e) {
      print('Exception while connecting: $e');
      return;
    }

    if (client.connectionStatus?.state == MqttConnectionState.connected) {
      isConnected = true;
      statusText = "Connected";
      client.subscribe('ugar', MqttQos.atLeastOnce);
      client.updates?.listen((List<MqttReceivedMessage<MqttMessage?>> c) {
        final MqttPublishMessage message = c[0].payload as MqttPublishMessage;
        final payload =
            MqttPublishPayload.bytesToStringAsString(message.payload.message);
        onNewMessage?.call(payload);
      });
    } else {
      statusText = "Connection failed";
      client.disconnect();
    }
  }

  void onConnected() {
    print('MQTTManager::onConnected');
    statusText = "Connected";
    isConnected = true;
  }

  void onDisconnected() {
    print('MQTTManager::onDisconnected');
    statusText = "Disconnected";
    isConnected = false;
  }

  void onSubscribed(String topic) {
    print('MQTTManager::onSubscribed');
    statusText = "Subscribed to $topic";
  }

  void disconnect() {
    print('MQTTManager::disconnect');
    client.disconnect();
    onDisconnected();
  }
}
