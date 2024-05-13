using Service.DAL;
using Service.JsonService;
using Service.MqttMessageManager;
using TrackingSystemWeb.BackgroundServices;
using TrackingSystemWeb.Hubs;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddSingleton<MqttClientService>();
builder.Services.AddSingleton<MqttDal>();
builder.Services.AddSingleton<JsonDeserialize>();
builder.Services.AddSingleton<LocationHub>();

// Add services to the container.
builder.Services.AddControllersWithViews();

builder.Services.AddHostedService<MqttClientAPI>();

builder.Services.AddSignalR();

builder.Services.AddHsts(options =>
{
    options.MaxAge = TimeSpan.FromDays(365);
    options.IncludeSubDomains = true;
    options.Preload = true;
});

var app = builder.Build();

// Configure the HTTP request pipeline.
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Home/Error");
    // The default HSTS value is 30 days. You may want to change this for production scenarios, see https://aka.ms/aspnetcore-hsts.
    app.UseHsts();
}

app.UseHttpsRedirection();
app.UseStaticFiles();

app.UseRouting();

app.UseAuthorization();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}");

app.MapHub<LocationHub>("/LocationHub");

app.Run();