import { darkModeStyles } from './mapsTheme.js'

var map
var marker
var infowindow
var time
export function initMap (latData = 41.1082, lngData = 28.9784) {
    time = moment().format('YYYY-MM-DD T HH:mm:ss')
    map = new google.maps.Map(document.getElementById('map'), {
        center: { lat: latData, lng: lngData },
        zoom: 14,
        styles: darkModeStyles
    })
    marker = new google.maps.Marker({
        position: { lat: latData, lng: lngData },
        map: map,
        icon: {
            path: google.maps.SymbolPath.FORWARD_CLOSED_ARROW,
            scale: 4,
        }, // googlenin "gezi" simgesi
        title: 'Vehicle',
    })
    infowindow = new google.maps.InfoWindow()
    marker.addListener('click', function () {
        var konum = marker.getPosition()
        infowindow.setContent('Konum: ' + konum.toUrlValue(6) + '<br> Zaman: ' + time)
        infowindow.open(map, marker)
    })
}

export function updateMap (lat, lng, newTime) { //update maps
    time = newTime
    var newLatLng = new google.maps.LatLng(lat, lng)
    marker.setPosition(newLatLng)
    map.panTo(newLatLng)
    var contentString = 'Konum: ' + newLatLng.toUrlValue(6) + '<br> Zaman: ' + newTime
    infowindow.setContent(contentString)
    if (infowindow.getMap()) {
        infowindow.open(map, marker)
    }
}