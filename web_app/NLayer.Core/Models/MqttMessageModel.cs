namespace NLayer.Core.Models
{
    public class MqttMessageModel:EventArgs
    {
        public string MessagePayload { get; set; }
        public string Topic { get; set; }

        public MqttMessageModel(string topic, string messagePayload)
        {
            Topic = topic;
            MessagePayload = messagePayload;
        }
    }
}
