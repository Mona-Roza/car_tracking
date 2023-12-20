import 'package:flutter/material.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Uygulaması',
      theme: ThemeData(
        primarySwatch: Colors.blue,
        visualDensity: VisualDensity.adaptivePlatformDensity,
      ),
      home: AnaSayfa(),
    );
  }
}

class AnaSayfa extends StatelessWidget {
  final List<String> araclar = ['Araç 1', 'Araç 2', 'Araç 3', 'Araç 4', 'Araç 5'];
  String seciliArac = 'Araç 1'; // Varsayılan olarak seçili araç

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Ana Sayfa'),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            Text(
              'Araç Seçiniz:',
              style: TextStyle(fontSize: 18),
            ),
            SizedBox(height: 10),
            DropdownButton<String>(
              value: seciliArac,
              onChanged: (String? yeniDeger) {
                if (yeniDeger != null) {
                  // Seçili değeri güncelle
                  seciliArac = yeniDeger;
                }
              },
              items: araclar.map<DropdownMenuItem<String>>((String arac) {
                return DropdownMenuItem<String>(
                  value: arac,
                  child: Text(arac),
                );
              }).toList(),
            ),
          ],
        ),
      ),
    );
  }
}

