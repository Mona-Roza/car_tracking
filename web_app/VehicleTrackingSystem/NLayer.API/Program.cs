using NLayer.API.BackgroundServices;
using NLayer.API.Hubs;
using NLayer.Repository.DAL;
using NLayer.Repository.MqttMessageManager;
using NLayer.Service.JsonService;

var builder = WebApplication.CreateBuilder(args);
// Add services to the container.
builder.Services.AddSingleton<MqttClientService>();
builder.Services.AddSingleton<MqttDal>();
builder.Services.AddSingleton<JsonDeserialize>();
builder.Services.AddSingleton<LocationHub>();

builder.Services.AddCors(options =>
{
    options.AddPolicy("CorsPolicy", builder =>
    {
        builder.WithOrigins("https://localhost:44388", "https://localhost:44361").AllowAnyHeader().AllowAnyMethod().AllowCredentials();
    });
});

builder.Services.AddHostedService<MqttClientAPI>();

builder.Services.AddControllers();
// Learn more about configuring Swagger/OpenAPI at https://aka.ms/aspnetcore/swashbuckle
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

builder.Services.AddSignalR();

var app = builder.Build();

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}

app.UseHttpsRedirection();

app.UseCors("CorsPolicy");

app.UseAuthorization();

app.MapControllers();

app.MapHub<LocationHub>("/LocationHub");

app.Run();