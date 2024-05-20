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
    }
}