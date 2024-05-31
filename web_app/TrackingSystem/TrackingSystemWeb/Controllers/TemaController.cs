using Microsoft.AspNetCore.Mvc;

namespace TrackingSystemWeb.Controllers
{
    public class TemaController : BaseController
    {
        public IActionResult Tables()
        {
            return View();
        }

        public IActionResult Charts()
        {
            return View();
        }

        public IActionResult Chart()
        {
            return View();
        }

        public IActionResult Index()
        {
            return View();
        }
    }
}