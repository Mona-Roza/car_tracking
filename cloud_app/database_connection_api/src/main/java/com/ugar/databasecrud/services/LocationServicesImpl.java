package com.ugar.databasecrud.services;

import com.ugar.databasecrud.entity.Location;
import com.ugar.databasecrud.repository.LocationRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class LocationServicesImpl implements LocationServices {
    @Autowired
    private LocationRepository locationRepository;
    @Override
    public List<Location> getLocationList() {
        return (List<Location>) locationRepository.findAll();
    }

    @Override
    public Location findByTimestamp(String timestamp) {

        return locationRepository.findByTimestamp(timestamp);

    }
}
