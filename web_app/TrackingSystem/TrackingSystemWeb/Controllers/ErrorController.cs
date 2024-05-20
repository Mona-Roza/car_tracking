using Microsoft.AspNetCore.Mvc;

namespace TrackingSystemWeb.Controllers
{
    public class ErrController : BaseController
    {
        public IActionResult Err401()
        {
            return View();
        }

        public IActionResult Err404()
        {
            return View();
        }

        public IActionResult Err500()
        {
            return View();
        }
    }
}