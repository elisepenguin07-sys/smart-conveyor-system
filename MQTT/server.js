import express from 'express';
import mqtt from 'mqtt';
import path from 'path';
import { fileURLToPath } from 'url';

const app = express();
const PORT = 8181;
app.use(express.json());

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const host = 'be899af830ab4926b516cded3f79cab4.s1.eu.hivemq.cloud';
const port = 8883;
const clientId = 'Nodejs_receiver' + Math.random().toString(16).slice(3);
const connectUrl = `mqtts://${host}:${port}`;
const options = {
    clientId,
    clean: true,
    connectTimeout: 4000,
    username: 'test',
    password: 'ee03101004',
    reconnectPeriod: 1000,
}

console.log('Connecting to HiveMQ Cloud MQTT broker...');
const client = mqtt.connect(connectUrl, options);
const TARGET_TOPIC = '/api/values';

let sendDate = {
    variable_a: 0,
    variable_b: 0,
    variable_c: 0
};

client.on('connect', () => {
    console.log('Connected to HiveMQ Broker!');

    client.subscribe(TARGET_TOPIC, { qos: 0 }, (err) => {
        if (!err) {
            console.log(`Successfully subscribed to topic: ${TARGET_TOPIC}`);
        }
        else {
            console.error(`Failed to subscribe to topic: ${TARGET_TOPIC}`, err);
        }
    })
});


client.on('message', (topic, payload) => {
    console.log(`Received message from topic ${topic}: ${payload.toString()}`);
    const messageStr = payload.toString();
    console.log('Received message:', messageStr);

    try {
        const jsonData = JSON.parse(messageStr);
        sendDate.variable_a = jsonData.variable_a;
        sendDate.variable_b = jsonData.variable_b;
        sendDate.variable_c = jsonData.variable_c;
        console.log('Updated Data:', sendDate);
    }
    catch (e) {
        console.log('NON-JSON format');
    }
})


client.on('error', (error) => {
    console.error('Connection error:', error);
});

// 託管 public 靜態網頁資料夾
app.use(express.static(path.join(__dirname, 'public')));

app.get('/api/values', (req, res) => {
    res.json(sendDate);
});

app.post('/mqtt', async (req, res) => {
    if (req.body.variable_a !== undefined) sendDate.variable_a = req.body.variable_a;
    if (req.body.variable_b !== undefined) sendDate.variable_b = req.body.variable_b;
    if (req.body.variable_c !== undefined) sendDate.variable_c = req.body.variable_c;

    console.log('收到新資料並已更新：', sendDate);
    res.json(sendDate);

});

// 💡 關鍵：一定要有這行，伺服器才會正式啟動並開始監聽！
app.listen(PORT, () => {
    console.log(`Server is running at http://localhost:${PORT}`);
});