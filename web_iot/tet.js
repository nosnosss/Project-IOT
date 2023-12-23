const express = require('express');
const bodyParser = require('body-parser');
const mysql = require('mysql');
const moment = require('moment-timezone');

const app = express();
const port = 3000;

app.use(bodyParser.urlencoded({ extended: true }));
app.set('view engine', 'ejs'); // Đặt engine mặc định là EJS

// Đặt múi giờ cho Việt Nam (Asia/Ho_Chi_Minh)
moment.tz.setDefault('Asia/Ho_Chi_Minh');

const db = mysql.createConnection({
  host: 'localhost',
  user: 'root',
  password: '0987654321',
  database: 'smart_lock',
});

db.connect((err) => {
  if (err) {
    console.log('Error connecting to MySQL:', err);
  } else {
    console.log('Connected to MySQL');
  }
});

app.post('/record', (req, res) => {
  const cardID = req.body.cardID;
  const currentTime = moment().format('YYYY-MM-DD HH:mm:ss');

  const sql = 'INSERT INTO access_logs (card_id, access_time) VALUES (?, ?)';
  db.query(sql, [cardID, currentTime], (err, result) => {
    if (err) {
      console.log('Error inserting data into database:', err);
      res.status(500).send('Internal Server Error');
    } else {
      console.log('Data inserted into database');
      res.send('Data recorded successfully');
    }
  });
});

app.get('/history', (req, res) => {
  const sql = 'SELECT * FROM access_logs ORDER BY access_time DESC';
  db.query(sql, (err, rows) => {
    if (err) {
      console.log('Error retrieving data from database:', err);
      res.status(500).send('Internal Server Error');
    } else {
      res.render('history', { accessLogs: rows });
    }
  });
});

app.listen(port, () => {
  console.log(`Server is running on port ${port}`);
});
