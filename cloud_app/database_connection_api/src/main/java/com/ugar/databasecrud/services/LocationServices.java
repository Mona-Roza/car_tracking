package com.ugar.databasecrud.services;

import com.ugar.databasecrud.entity.Location;

import java.util.List;

public interface LocationServices {
    List<Location> getLocationList();
    Location findByTimestamp(String timestamp);

}
