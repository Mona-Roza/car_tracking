using Microsoft.AspNetCore.Mvc;

namespace TrackingSystemWeb.Controllers
{
    public class UserController : BaseController
    {
        public IActionResult Login()
        {
            return View();
        }

        public IActionResult Password()
        {
            return View();
        }

        public IActionResult Register()
        {
            return View();
        }
    }
}