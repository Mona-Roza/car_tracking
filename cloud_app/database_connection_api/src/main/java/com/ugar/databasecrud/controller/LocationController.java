package com.ugar.databasecrud.controller;

import com.ugar.databasecrud.entity.Location;
import com.ugar.databasecrud.exception.TimestampNotFoundException;
import com.ugar.databasecrud.repository.LocationRepository;
import com.ugar.databasecrud.services.LocationServices;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.server.ResponseStatusException;

import java.util.List;

@RequestMapping("/api")
@RestController
public class LocationController {

    @Autowired
    LocationServices locationServices;

    @GetMapping("/locations")
    public ResponseEntity<List<Location>> getLocationList(){
        return ResponseEntity.ok(locationServices.getLocationList());
    }
    @GetMapping("/locations/{timestamp}")
    public Location getLocation(@PathVariable String timestamp){
        if(timestamp == null){
            throw new IllegalArgumentException("Timestamp parameter is required");
        }

        Location theLocation = locationServices.findByTimestamp(timestamp);
        if(theLocation == null){
            throw new ResponseStatusException(HttpStatus.NOT_FOUND, "Location not found with timestamp: " + timestamp);
        }

        return theLocation;
    }
}
