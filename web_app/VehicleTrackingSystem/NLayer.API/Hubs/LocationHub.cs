﻿using Microsoft.AspNetCore.SignalR;
using NLayer.Core.Models;

namespace NLayer.API.Hubs
{
    public class LocationHub : Hub
    {
        private static List<LocationModel> locations { get; set; } = new List<LocationModel>();
        private static int ClientCount { get; set; } = 0;
        public async Task SendName(LocationModel location)
        {
            locations.Add(location);
            Console.WriteLine("Latitude:" + location.Latitude + " / Longitude:" + location.Longitude + " / Date:" + location.Timestamp);
            await Clients.All.SendAsync("ReceiveName", location);
        }

        public async Task GetNames()
        {
            await Clients.All.SendAsync("ReceiveNames", locations);
        }
        public override async Task OnConnectedAsync()
        {
            ClientCount++;
            await Clients.All.SendAsync("ReceiveClientCount", ClientCount);
            await base.OnConnectedAsync();
        }
        public override async Task OnDisconnectedAsync(Exception? exception)
        {
            ClientCount--;
            await Clients.All.SendAsync("ReceiveClientCount", ClientCount);
            await base.OnDisconnectedAsync(exception);
        }


    }
}