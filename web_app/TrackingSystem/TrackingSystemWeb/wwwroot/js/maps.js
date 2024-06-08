import { darkModeStyles } from './mapsTheme.js'

var map
var marker
var newMarker
var markers = []
var infowindow
var time
let historyClicked = false
export function initMap (latData = 41.1082, lngData = 28.9784) {
    time = moment().format('YYYY-MM-DD HH:mm:ss')
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
        customTime: time
    })
    infowindow = new google.maps.InfoWindow()
    marker.addListener('click', function () {
        var konum = marker.getPosition()
        infowindow.setContent('Konum: ' + konum.toUrlValue(6) + '<br> Zaman: ' + time)
        infowindow.open(map, marker)
    })
}

var apiKey = 'AIzaSyDQlZlLgO2Luut0trOyN7ESj170hGUcV6Q'
export function loadScript () {
    var script = document.createElement('script')
    script.type = 'text/javascript'
    script.src = 'https://maps.googleapis.com/maps/api/js?key=' + apiKey + '&callback=initMap&loading=async'
    document.body.appendChild(script)
}

export function updateMap (location) { //update maps
    if (historyClicked) {
        loadNewHistoryLocation()
    }
    time = moment(location.timestamp).format('YYYY-MM-DD HH:mm:ss')
    var newLatLng = new google.maps.LatLng(location.latitude, location.longitude)
    marker.setPosition(newLatLng)
    map.panTo(newLatLng)
    marker.customTime = time
    var contentString = 'Konum: ' + newLatLng.toUrlValue(6) + '<br> Zaman: ' + time
    infowindow.setContent(contentString)
    if (infowindow.getMap()) {
        infowindow.open(map, marker)
    }
}

function loadNewHistoryLocation () {
    var position = marker.getPosition()
    console.log(marker.customTime)
    var newLatLng = new google.maps.LatLng(position.lat(), position.lng())
    newMarker = new google.maps.Marker({
        position: newLatLng,
        map: map,
        title: 'Konum: ' + newLatLng.toUrlValue(6)
    })
    markers.push(newMarker)
    addMarker(newMarker, marker.customTime)
}

export function loadHistoryLocations () {
    $(document).on('click', '#history', function () {
        if (historyClicked == false) {
            historyClicked = true
            $.ajax({
                type: 'GET', // HTTP method
                url: 'http://localhost:5210/Home/GetLocationHistory', // MVC Action Method URL
                dataType: 'json', // Data type expected from the server
                success: function (response) {
                    // Handle successful response
                    $.each(response, function (index, item) {
                        var time = item.timestamp
                        var newLatLng = new google.maps.LatLng(item.latitude, item.longitude)

                        newMarker = new google.maps.Marker({
                            position: newLatLng,
                            map: map,
                            title: 'Konum: ' + newLatLng.toUrlValue(6)
                        })
                        markers.push(newMarker)

                        addMarker(newMarker, time)
                    })

                    if (response.length > 0) {
                        map.panTo(new google.maps.LatLng(response[0].latitude, response[0].longitude))
                    }
                },
                error: function (xhr, status, error) {
                    // Handle errors
                    $('#content').html('Error: ' + error)
                }
            })
            document.getElementById('history').textContent = 'Gecmisi Kapat'
        } else if (historyClicked == true) {
            // Marker'larý haritadan kaldýr
            markers.forEach(function (marker) {
                marker.setMap(null) // Marker'ý haritadan kaldýr
            })
            markers = [] // Marker listesini sýfýrla
            historyClicked = false
            document.getElementById('history').textContent = 'Gecmisi Goster'
        }
    })
}
function addMarker (newMarker, time) {
    var contentString = 'Konum: ' + newMarker.getPosition().toUrlValue(6) + '<br> Zaman: ' + moment(time).format('YYYY-MM-DD HH:mm:ss')
    newMarker.addListener('click', function () {
        infowindow.setContent(contentString)
        infowindow.open(map, newMarker)
    })
}