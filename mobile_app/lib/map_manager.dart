import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';

class MapManager {
  Set<Marker> markers = {};
  Set<Polyline> polylines = {};
  GoogleMapController? mapController;
  List<LatLng> routePoints = [];

  void onMapCreated(GoogleMapController controller) {
    mapController = controller;
  }

  Future<BitmapDescriptor> createCustomMarkerIcon() async {
    return await BitmapDescriptor.fromAssetImage(
        ImageConfiguration(
          devicePixelRatio: 5,

        ),
        'assets/icon/mustafa2.png'); // Yerel asset yolunuza göre ayarlayın
  }

  Future<void> updateMarkers(String message) async {
    Map<String, dynamic> json = jsonDecode(message);
    double lat = json['latitude'];
    double lon = json['longitude'];
    LatLng newPoint = LatLng(lat, lon);

    Marker newMarker = Marker(
      markerId: MarkerId(json['timestamp']),
      position: newPoint,
      icon: await createCustomMarkerIcon(),
      infoWindow: InfoWindow(
        title: 'Timestamp: ${json['timestamp']}',
        snippet: 'Lat: $lat, Lon: $lon',
      ),
    );

    markers.add(newMarker);
    updateRoute(newPoint);

    mapController?.animateCamera(
      CameraUpdate.newCameraPosition(
        CameraPosition(
          target: newPoint,
          zoom: 14.0,
        ),
      ),
    );
  }

  void updateRoute(LatLng newPoint) {
    routePoints.add(newPoint);
    final String polylineIdVal = 'polyline_${routePoints.length}';
    polylines.add(
      Polyline(
        polylineId: PolylineId(polylineIdVal),
        width: 5,
        color: Colors.blue,
        points: routePoints,
      ),
    );
  }
}
