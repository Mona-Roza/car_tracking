using AutoMapper;
using Core.Models;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using System.Net.Http.Headers;
using System.Text.Json;
using TrackingSystemWeb.ViewModel;

namespace TrackingSystemWeb.Controllers
{
    [Authorize]
    public class HomeController : BaseController
    {
        private readonly ILogger<HomeController> _logger;
        private readonly IHttpClientFactory _httpClientFactory;
        private readonly IMapper _mapper;

        public HomeController(ILogger<HomeController> logger, IHttpClientFactory httpClientFactory, IMapper mapper)
        {
            _logger = logger;
            _httpClientFactory = httpClientFactory;
            _mapper = mapper;
        }

        public IActionResult Index()
        {
            return View();
        }

        public async Task<IActionResult> Privacy()
        {
            var locations = new List<LocationModel>();

            var token = User.Claims.FirstOrDefault(x => x.Type == "JWToken")?.Value;
            if (token == null)
            {
                return RedirectToAction("Login", "Account");
            }
            using (var httpClient = new HttpClient())
            {
                httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", token);
                using (var response = await httpClient.GetAsync("http://localhost:5278/api/Location"))
                {
                    string apiResponse = await response.Content.ReadAsStringAsync();
                    locations = JsonSerializer.Deserialize<List<LocationModel>>(apiResponse);
                }
            }
            var locationViewModels = _mapper.Map<List<LocationsViewModel>>(locations.ToList());
            return View(locationViewModels);
        }

        public IActionResult Tables()
        {
            return View();
        }

        public IActionResult Charts()
        {
            return View();
        }
    }
}