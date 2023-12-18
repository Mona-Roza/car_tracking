import boto3
import json

def lambda_handler(event, context):
    # DynamoDB istemcisini başlat
    client = boto3.client('dynamodb')



    item = {
        'timestamp': {'S': event['timestamp']},
        'latitude': {'N': str(event['latitude'])},
        'longitude': {'N': str(event['longitude'])}
    }

    # Veriyi DynamoDB tablosuna yaz
    response = client.put_item(
        TableName='esp32Data_v1',  # Tablo adınız
        Item=item
    )

    # İşlem sonucunu döndür
    return {
        'statusCode': 200,
        'body': json.dumps('Item successfully written to DynamoDB')
    }
