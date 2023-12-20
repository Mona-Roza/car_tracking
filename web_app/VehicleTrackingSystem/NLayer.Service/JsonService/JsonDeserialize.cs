using Newtonsoft.Json;
using NLayer.Core.Models;
using NLayer.Repository.DAL;

namespace NLayer.Service.JsonService
{
    public class JsonDeserialize
    {
        private readonly MqttDal _mqttDal;
        private LocationModel _locationModel;
        public event EventHandler<MqttMessageModel> MessageReceivedMqtt;
        public event EventHandler<LocationModel> MessageReceivedLocation;
        public JsonDeserialize(MqttDal mqttDal)
        {
            _mqttDal = mqttDal;
        }
        public async Task GetMqttMessage(string topic)
        {
            await _mqttDal.GetMqttMessage(topic);
            _mqttDal.MessageReceived += Deserialize;
        }
        private void Deserialize(object sender, MqttMessageModel model)
        {
            try
            {
                _locationModel = JsonConvert.DeserializeObject<LocationModel>(model.MessagePayload);
                MessageReceivedLocation?.Invoke(this, _locationModel);
            }
            catch (Exception)
            {
                MessageReceivedMqtt?.Invoke(this, model);
            }
        }
    }
}
