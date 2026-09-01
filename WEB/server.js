import express from 'express';
import mqtt from 'mqtt';
import path from 'path';
import { fileURLToPath } from 'url';

const app = express();
const PORT = 8181;

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
app.use(express.json());


const host = 'a7e49771b2094445aff9bf69987ee553.s1.eu.hivemq.cloud';
const port = 8883;
const clientId = 'Nodejs_receiver' + Math.random().toString(16).slice(3);
const connectUrl = `mqtts://${host}:${port}`;

const options = {
    clientId,
    clean: true,
    connectTimeout: 4000,
    username: 'elise',
    password: 'ee03101004',
    reconnectPeriod: 1000,
};


console.log('Connecting to HiveMQ Cloud MQTT broker...');
const client = mqtt.connect(connectUrl, options);
const TARGET_TOPIC = '/vibrateData';


let sendDate = {
    rms: { x: 0, y: 0, z: 0 },
    fft: { x: "", y: "", z: "" },
    diagnosis: { reason: "", actions: [] }
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
    const messageStr = payload.toString();
    console.log('Received message:', messageStr);
    try {
        const jsonData = JSON.parse(messageStr);

        // 💡 確保這幾行有確實分開執行，用來更新全域變數
        if (jsonData.rms) sendDate.rms = jsonData.rms;
        if (jsonData.fft) sendDate.fft = jsonData.fft;
        if (jsonData.diagnosis) sendDate.diagnosis = jsonData.diagnosis;

        console.log('Updated Data successfully:', sendDate);
    } catch (e) {
        console.log('NON-JSON format', e);
    }
})


client.on('error', (err) => {
    console.error('❌ [Node.js] MQTT 連線錯誤:', err.message);
});

client.on('error', (error) => {
    console.error('Connection error:', error);
});

// 託管 public 靜態網頁資料夾
app.use(express.static(path.join(__dirname, 'public')));

app.get('/api/values', (req, res) => {
    res.json(sendDate);
});


app.post('/mqtt', async (req, res) => {
    if (req.body.rms !== undefined) sendDate.rms = req.body.rms;
    if (req.body.fft !== undefined) sendDate.fft = req.body.fft;
    if (req.body.diagnosis !== undefined) sendDate.diagnosis = req.body.diagnosis;
    console.log('收到新資料並已更新：', sendDate);
    res.json(sendDate);
});


app.listen(PORT, () => {
    console.log(`Server is running at http://localhost:${PORT}`);
});


