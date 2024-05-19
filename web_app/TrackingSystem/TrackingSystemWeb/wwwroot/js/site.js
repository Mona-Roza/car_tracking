import { serverConnection } from './serverConnection.js'
import { initMap, loadScript } from './maps.js'
//maps ekranda varmı kontrolü
if ($('#map').length > 0) {
    window.initMap = initMap
}

$(document).ready(() => {
    serverConnection()

    loadScript()
})