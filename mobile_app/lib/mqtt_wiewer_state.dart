
import 'package:flutter/material.dart';
import 'mqtt_manager.dart';
import 'google_maps_widget.dart';
import 'map_manager.dart';

class MQTTViewer extends StatefulWidget {
  @override
  _MQTTViewerState createState() => _MQTTViewerState();
}

class _MQTTViewerState extends State<MQTTViewer> {
  late MQTTManager mqttManager;
  String lastMessage = "No messages yet.";
  MapManager mapManager = MapManager();

  @override
  void initState() {
    super.initState();
    mqttManager = MQTTManager(
      onNewMessage: (String message) {
        setState(() {
          lastMessage = message;
          mapManager.updateMarkers(message);
        });
      },
    );
  }

  void toggleConnection() {
    if (mqttManager.isConnected) {
      mqttManager.disconnect();
    } else {
      mqttManager.initializeMQTTClient();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Araç Takip Sistemi'),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Text(
              'Connection Status: ${mqttManager.statusText}',
              style: TextStyle(fontSize: 20),
            ),
            SizedBox(height: 24),
            Text(
              'Last Message: $lastMessage',
              style: TextStyle(fontSize: 16),
            ),
            ElevatedButton(
              onPressed: toggleConnection,
              child: Text(mqttManager.isConnected ? 'Disconnect' : 'Connect'),
            ),
            if (!mqttManager.isConnected)
              Padding(
                padding: EdgeInsets.all(20),
                child: CircularProgressIndicator(),
              ),
            Expanded(
              child: GoogleMapWidget(
                markers: mapManager.markers,
                polylines: mapManager.polylines,
                onMapCreated: mapManager.onMapCreated,
              ),
            ),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    mqttManager.disconnect();
    super.dispose();
  }
}