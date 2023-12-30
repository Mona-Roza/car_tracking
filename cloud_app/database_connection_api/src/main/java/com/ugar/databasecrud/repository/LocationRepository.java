package com.ugar.databasecrud.repository;

import com.ugar.databasecrud.entity.Location;
import org.socialsignin.spring.data.dynamodb.repository.EnableScan;
import org.springframework.data.repository.CrudRepository;
import org.springframework.stereotype.Repository;

@Repository
@EnableScan
public interface LocationRepository extends CrudRepository<Location,String> {
    Location findByTimestamp(String timestamp);
}
