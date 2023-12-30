import 'package:flutter/material.dart';
import 'package:google_maps_flutter/google_maps_flutter.dart';

class GoogleMapWidget extends StatefulWidget {
  final Set<Marker> markers;
  final Set<Polyline> polylines;
  final Function(GoogleMapController) onMapCreated;

  GoogleMapWidget({required this.markers, required this.polylines, required this.onMapCreated});

  @override
  _GoogleMapWidgetState createState() => _GoogleMapWidgetState();
}

class _GoogleMapWidgetState extends State<GoogleMapWidget> {
  @override
  Widget build(BuildContext context) {
    return GoogleMap(
      onMapCreated: widget.onMapCreated,
      markers: widget.markers,
      polylines: widget.polylines,
      initialCameraPosition: CameraPosition(
        target: LatLng(40.993908896415576, 27.584095),
        zoom: 11.0,
      ),
    );
  }
}
