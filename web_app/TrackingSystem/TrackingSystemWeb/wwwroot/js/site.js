import { serverConnection } from './serverConnection.js'
import { initMap } from './maps.js'
//maps api
if ($('#map').length > 0) {
    window.initMap = initMap
}

$(document).ready(() => {
    serverConnection()
    var apikey = 'AIzaSyDQlZlLgO2Luut0trOyN7ESj170hGUcV6Q'
    function loadScript () {
        var script = document.createElement('script')
        script.type = 'text/javascript'
        script.src = 'https://maps.googleapis.com/maps/api/js?key=' + apikey + '&callback=initMap&loading=async'
        document.body.appendChild(script)
    }
    loadScript()
})