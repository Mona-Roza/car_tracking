using Newtonsoft.Json;
using Newtonsoft.Json.Converters;

namespace Core.Models
{
    public class BaseEntity
    {
        //public int Id { get; set; }
        [JsonConverter(typeof(CustomDateTimeConverter))]
        public DateTime Timestamp { get; set; } = DateTime.Now;
    }
}

public class CustomDateTimeConverter : IsoDateTimeConverter
{
    public CustomDateTimeConverter()
    {
        DateTimeFormat = "yyyy-MM-ddTHH:mm:ss";
    }
}