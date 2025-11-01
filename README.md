# DYBRO: Die! You, Brain Rot


Disusun Oleh :
Ignatius Ivan Wijaya
Marcellino Asanuddin
Yazid Maulana Rizky
Cathlyn Shanice Dharmawan

Dosen Pembimbing:
Bakti Amirul Jabar, S.Si., M.Kom.




Latar Belakang
Pandemi COVID-19 telah mempercepat adopsi E-Learning (Pembelajaran Jarak Jauh) secara masif di seluruh institusi pendidikan. Meskipun mengutamakan fleksibilitas, model pembelajaran ini membawa tantangan signifikan, terutama dalam hal menjaga konsentrasi siswa. Tanpa pengawasan langsung seperti di kelas fisik, siswa dihadapkan pada berbagai distraksi digital. Perangkat seperti laptop atau handphone yang digunakan untuk mengakses materi pelajaran juga merupakan perangkat yang sama untuk mengakses media sosial, game, dan notifikasi tanpa henti. Selain distraksi digital, faktor lingkungan fisik seperti suhu ruangan, kelembapan, dan pencahayaan juga berperan krusial dalam menjaga kenyamanan dan kemampuan kognitif siswa
Kurangnya fokus ini berdampak langsung pada efektivitas penyerapan materi, membuat siswa rentan terhadap prokrasitinasi dan passive learning. Siswa seringkali pasif di depan layar, hanya hadir secara fisik namun tidak terlibat secara mental. Kami mengidentifikasi adanya kebutuhan akan sebuah alat atau sistem cerdas yang dapat bertindak sebagai "rekan belajar" atau "pengawas digital". Dimana alat ini, harus mampu “merasakan” (sense) status belajar siswa secara holistik, tidak hanya sekedar aktivitasnya, namun juga kenyamanan lingkungannya. Oleh karena itu, kami mengusulkan DYBRO, sebuah sistem berbasis Internet of Things (IoT) yang dirancang khusus untuk memantau aktivitas fisik serta kenyamanan lingkungan yang dapat memberikan feedback multimodal guna membantu siswa tetap fokus dan sehat.
Tujuan
Tujuan umum dari project DYBRO adalah meningkatkan konsentrasi dan keterlibatan siswa selama sesi belajar mandiri melalui sistem pemantauan fokus dan lingkungan yang terintegrasi. Tujuan khusus dari project DYBRO adalah sebagai berikut : 
Merancang dan membangun prototype perangkat IoT DYBRO menggunakan mikrokontroler ESP32, sensor gerak (IR Obstacle), sensor lingkungan (DHT11), dan input tombol
Mengembangakn algoritma berbasis state-machine (Pomodoro) sebagai pengelola siklus belajar dan istirahat secara otomatis.
Mengimplementasikan sistem feedback multi-modal menggunakan RGB LED dan Buzzer
Menguji efektivitas perangkat dalam skenario pembelajaran yang disimulasikan.
Komponen yang Digunakan
Berikut merupakan komponen yang kami gunakan dalam project DYBRO : 
Bagian
Nama Komponen
Jumlah
Fungsi
Mikrokontroler
ESP32 Dev Kit
1
Sebagai pemrosesan utama logika sistem DYBRO
ESP32 Cam
1
Untuk pengembangan lanjutan deteksi fokus berbasis visual
Sensor / Input 
IR Obstacle Sensor
1
Mendeteksi gerakan tubuh, terhubung ke SENSOR_PIN
Sensor Suhu & Kelembapan DHT11
1
Mendeteksi kenyamanan ruangan siswa
Push Button 
1
Untuk memulai/ mengontrol state sistem
Kamera
1
Menangkap visual siswa, yang kemudian digunakan untuk analisa fokus dan aktvitas dalam pembelajaran
Indikator Visual / Output
RGB LED
1
Indikator status visual: Fokus, Tidak Fokus, Istirahat
Indikator Audio / Output 
Buzzer Aktif
1
Indikator status audio
Komponen Pendukung
Breadboard Besar
1


Breadboard Kecil
3


Kabel Jumper
9
M:F = 6, M:M = 3


Skema Rangkaian
Sensor DHT11 (Input): Pin DATA terhubung ke GPIO 4.
IR Obstacle Sensor (Input): Pin OUT terhubung ke GPIO 5 (didefinisikan sebagai SENSOR_PIN).
Button (Input): Satu kaki terhubung ke GPIO 14 (didefinisikan sebagai BUTTON_PIN). Kaki lainnya terhubung ke GND. (Kode menggunakan INPUT_PULLUP internal).
RGB LED (Output):
Pin Merah terhubung ke GPIO 27 (RED_PIN).
Pin Hijau terhubung ke GPIO 26 (GREEN_PIN).
Pin Biru terhubung ke GPIO 25 (BLUE_PIN).
(Asumsi Common Cathode terhubung ke GND, berdasarkan logika analogWrite di kode).
Buzzer (Output): Pin Positif terhubung ke GPIO 23 (BUZZER_PIN). Pin Negatif ke GND.
Power: Papan ESP32 ditenagai melalui kabel USB.
Penjelasan Logika Program
Untuk file Arduino bisa didownload melalui: https://drive.google.com/file/d/18hlGZqtL4GP75XyLtzHIVut7efQncyba/view?usp=sharing

Inisialisasi (void setup()):
Mengatur pin Input dan Output, yaitu:
Input: BUTTON_PIN, SENSOR_PIN, DHTPIN
Output: RED_PIN, GREEN_PIN, BLUE_PIN, BUZZER_PIN.
Menginisialisasi sensor DHT11 serta koneksi WiFi dan MQTT.
WiFi dikonfigurasi menggunakan SSID dan password, lalu terhubung ke broker MQTT broker.hivemq.com di port 1883.
Sensor analog (IR Obstacle) dikalibrasi terlebih dahulu untuk membaca nilai awal stabil.

Loop Utama (void loop()):
Loop utama menjalankan tiga bagian besar secara non-blocking:\
Handler MQTT Non-Blocking
Mengecek koneksi MQTT setiap 5 detik (lastMqttReconnectAttempt).
 Jika terputus, sistem mencoba reconnect otomatis.
Menjalankan client.loop() untuk menjaga komunikasi MQTT tetap aktif.
Setiap 5 detik (mqttPublishInterval), sistem mengirim data status ke broker MQTT, berisi:
state (IDLE, SESSION, BREAK, PAUSED)
timeRemaining_s (sisa waktu dalam detik)
isFokus (true/false)
temperature (dari DHT11)
sensorNoise (fluktuasi dari sensor IR)

State Machine
Program utama dikontrol oleh state machine, dengan 4 state utama:
IDLE
SESSION
BREAK
PAUSED
Masing-masing state diproses oleh fungsi handler:
handleIdleState()
handleSessionState()
handleBreakState()
handlePausedState()

Pembacaan Sensor dan Logging
Sensor IR dibaca setiap 100 ms (sensorReadInterval) dan disimpan dalam array.
Setiap 2 detik (lastSensorCalc), sistem menghitung standar deviasi sensor (sensorNoise).
Jika sensorNoise > NOISE_THRESHOLD, pengguna dianggap sedang aktif/fokus.
Jika tidak, pengguna dianggap diam/tidak fokus.
Suhu ruangan dari DHT11 juga diperbarui secara berkala.
Sistem juga mencetak log ke Serial Monitor setiap 2 detik (lastInputLog) untuk debugging: Status state, tombol, sensor, dan suhu.

handleIdleState() (State: IDLE):
LED dalam keadaan mati (setColor(0,0,0)).
Menunggu input tombol ditekan (Single Click) untuk memulai sesi.
Transisi: Jika tombol ditekan, sistem memainkan playStartSound() (beep ganda), mengatur ulang timer sessionStart, dan berpindah ke currentState = SESSION.
handleSessionState() (State: SESSION):
Timer Sesi: Cek elapsed = millis() - sessionStart.
Transisi  ke BREAK: 
Jika elapsed >= sessionDuration (25 menit), sistem berpindah ke currentState = BREAK, memainkan playEndSound() (beep panjang), set LED ke Biru, dan menampilkan "Istirahat!".
Logika Sesi (Berjalan):
Variabel personDetected (dari sensor IR) dicek.
JIKA personDetected == true (Fokus):
LED di-set ke Hijau (setColor(0, 255, 0)).
Peringatan Suhu: Jika lastTemp > 30.0 (dari DHT11)
JIKA personDetected == false (Tidak Fokus):
LED di-set ke Merah (setColor(255, 0, 0)).
Sistem memainkan playNudgeSound() (beep pendek) untuk mengingatkan pengguna.
handleBreakState() (State: BREAK):
LED di-set ke Biru (setColor(0, 0, 255)).
Timer Istirahat: Cek elapsed = millis() - sessionStart.
Transisi (Selesai): Jika elapsed >= breakDuration (5 menit), sistem berpindah kembali ke currentState = IDLE, memainkan playEndSound(), dan mematikan LED, siap untuk sesi baru.
handlePausedState() (State: PAUSED)
LED berwarna Oranye menandakan jeda.
Jika pause berlangsung lebih dari 2 menit (pauseAutoResume),
sistem otomatis melanjutkan sesi ke STATE = SESSION dan memainkan playStartSound().
Interaksi Tombol
Single Click : Mulai sesi (dari IDLE Ke SESSION)
Double Click : Pause/ Resume sesi (SESSION ke PAUSED)
Long Press : Stop sesi apapun (SESSION/BREAK/PAUSED ke IDLE)

Fungsi Helper (Suara & Tampilan):
calculateStdDev(arr, n):
 Menghitung fluktuasi data sensor IR untuk mendeteksi aktivitas tubuh pengguna.
setColor(r, g, b):
Mengatur warna RGB LED berdasarkan status (Hijau = Fokus, Merah = Tidak Fokus, Biru = Istirahat, Oranye = Pause).
formatTime(ms):
 Mengubah milidetik menjadi format menit:detik (“MM:SS”).
playNudgeSound(), playStartSound(), playEndSound():
 Memberikan umpan balik suara (beep pendek, ganda, atau panjang) sesuai event.
Hasil Pengujian
No.
Skenario Pengujian
Input / Aktivitas Pengguna / Initial State
Output yang Diharapkan
Output Aktual
Kesimpulan
1.
Idle (Menunggu)
currentState = IDLE
RGB LED: MATI 
RGB LED: MATI
ACCEPT
2.
Mulai Sesi
currentState = IDLE
Tekan Tombol saat IDLE
currentState = SESSION 
RGB LED: HIJAU 
Buzzer: Beep Ganda (Start)
currentState = SESSION 
RGB LED: HIJAU 
Buzzer: Beep Ganda (Start)
ACCEPT
3.
Sesi (Fokus)
currentState = SESSION Sensor IR: Noise > 0 → Ada orang/objek
RGB LED: HIJAU
Buzzer: Diam
RGB LED: HIJAU
Buzzer: Diam
ACCEPT
4.
Sesi (Tidak Fokus)
currentState = SESSION Sensor IR: Noise == 0 → Ada orang/objek
RGB LED: MERAH Buzzer: Beep Pendek (Nudge)
RGB LED: MERAH Buzzer: Beep Pendek (Nudge)
ACCEPT
5.
Sesi (Gerah)
currentState = SESSION PIR: HIGH DHT11: Suhu > 30°C
RGB LED: HIJAU (Tetap fokus)
RGB LED: HIJAU (Tetap fokus)
ACCEPT
6.
Transisi ke Istirahat
currentState = SESSION Timer Sesi > 25 Menit
currentState = BREAK 
RGB LED: BIRU Buzzer: Beep Panjang (End)
currentState = BREAK 
RGB LED: BIRU Buzzer: Beep Panjang (End)
ACCEPT
7.
Transisi ke Idle
currentState = BREAK Timer Istirahat > 5 Menit
currentState = IDLE 
RGB LED: MATI
Buzzer: Beep Panjang (End)
currentState = IDLE 
RGB LED: MATI
Buzzer: Beep Panjang (End)
ACCEPT

Kesimpulan Pengujian: Logika state-machine Pomodoro berfungsi sesuai rancangan. Sistem dapat membedakan antara fokus (Hijau), tidak fokus (Merah), dan istirahat (Biru), serta memberikan peringatan lingkungan (suhu) dan audio (nudge/start/end) yang spesifik.
Pembagian Peran
Ignatius Ivan Wijaya
Bertanggung jawab atas manajemen waktu (timeline) proyek.
Mengkoordinasi komunikasi antar anggota tim.
Menganalisis kebutuhan sistem
Marcellino Asanuddin
Bertanggung jawab atas riset dan pemilihan komponen perangkat keras.
Merancang skema rangkaian (circuit diagram).
Melakukan perakitan fisik prototipe  dan troubleshooting perangkat keras.
Yazid Maulana Rizky
Bertanggung jawab atas penulisan kode (sketch) ESP32.
Mengimplementasikan logika state-machine Pomodoro dan driver sensor/aktuator.
Melakukan debugging dan upload program ke mikrokontroler.
Cathlyn Shanice Dharmawan
Bertanggung jawab dalam menyusun laporan akhir. 
Menyusun alur logika proposal
Merancang skenario pengujian (seperti tabel di bagian 6) 
Pengembangan Lanjutan
Project DYBRO ini memiliki potensi besar untuk dikembangkan lebih lanjut : 
Integrasi Computer Vision (CV): Memanfaatkan ESP32 CAM yang sudah ada dalam daftar komponen. Sensor IR dibuat hanya untuk mendeteksi kehadiran seseorang, bukan sebagai indikator fokus atau aktivitas. Sedangkan kamera digunakan untuk menganalisis tingkat fokus pengguna, misalnya melalui deteksi arah pandangan (gaze-tracking) atau deteksi penggunaan gadget. Dengan pendekatan ini, sistem dapat membedakan antara “diam karena membaca” dan “diam karena melamun.”
Konektivitas IoT & Dashboard: Memanfaatkan WiFi internal ESP32 untuk menghubungkan perangkat ke cloud (misal, Firebase). Data (suhu, jam fokus, jumlah "nudge") dapat ditampilkan pada dashboard web/aplikasi seluler untuk melacak produktivitas.
Gamifikasi: Menambahkan sistem poin pada dashboard. Siswa mendapatkan "Focus Points" untuk setiap sesi (RGB Hijau) tanpa nudge (RGB Merah).
Personalisasi: Memungkinkan pengguna mengatur durasi Pomodoro (sesi/istirahat) dan threshold suhu melalui aplikasi seluler atau web dashboard.
Lampiran
https://drive.google.com/drive/folders/1o5Cx0W6EFn5sGVF_Z-SeLXALzSyNuDr6?usp=sharing

