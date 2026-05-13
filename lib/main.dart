import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setPreferredOrientations([DeviceOrientation.portraitUp]);
  runApp(UTMACHTimerApp());
}

class UTMACHTimerApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Contador 24 y 14 segundos',
      theme: ThemeData(primarySwatch: Colors.indigo),
      home: TimerPage(),
    );
  }
}

class TimerPage extends StatefulWidget {
  @override
  _TimerPageState createState() => _TimerPageState();
}

class _TimerPageState extends State<TimerPage> {
  BluetoothConnection? connection;
  int tiempoRestante = 0;
  int ultimoTiempoSeteado = 0; 
  Timer? _timer;
  
  String logStatus = "Modo sin conexión Bluetooth";
  final TextEditingController _timeController = TextEditingController();

  final Color azulUTMACH = const Color(0xFF4DA6FF);
  final Color naranjaUTMACH = const Color(0xFFFF8C29);

  // --- LÓGICA BLUETOOTH ULTRA-RÁPIDA ---
  void conectarBluetooth() async {
    try {
      List<BluetoothDevice> devices = await FlutterBluetoothSerial.instance.getBondedDevices();
      if (!mounted) return;

      BluetoothDevice? selectedDevice = await showDialog(
        context: context,
        builder: (context) => AlertDialog(
          title: const Text("Seleccionar Marcador"),
          content: SizedBox(
            width: double.maxFinite,
            child: ListView.builder(
              shrinkWrap: true,
              itemCount: devices.length,
              itemBuilder: (context, i) => ListTile(
                title: Text(devices[i].name ?? "Desconocido"),
                subtitle: Text(devices[i].address),
                onTap: () => Navigator.pop(context, devices[i]),
              ),
            ),
          ),
        ),
      );

      if (selectedDevice != null) {
        setState(() => logStatus = "Conectando...");
        connection = await BluetoothConnection.toAddress(selectedDevice.address);
        setState(() => logStatus = "Conectado a ${selectedDevice.name}");
      }
    } catch (e) {
      setState(() => logStatus = "Error de conexión");
    }
  }

  // LA MAGIA ESTÁ AQUÍ: Enviamos un Byte puro en lugar de texto
  void enviarComandoByte(int byteCmd, int seg) async {
    if (connection != null && connection!.isConnected) {
      // Uint8List convierte el número en un paquete binario perfecto de 8 bits
      connection!.output.add(Uint8List.fromList([byteCmd]));
      await connection!.output.allSent;
      
      setState(() {
        tiempoRestante = seg;
        logStatus = "Señal enviada: $byteCmd";
      });
    } else {
      setState(() {
        tiempoRestante = seg;
        logStatus = "Sin Bluetooth";
      });
    }
  }

  // --- CONTROLES DINÁMICOS ---
  void togglePlayPause() {
    if (_timer != null && _timer!.isActive) {
      _timer!.cancel();
      enviarComandoByte(251, tiempoRestante); // 251 = PAUSE
    } else {
      if (tiempoRestante > 0) {
        enviarComandoByte(250, tiempoRestante); // 250 = START
        _timer = Timer.periodic(const Duration(seconds: 1), (timer) {
          if (tiempoRestante > 0) {
            setState(() => tiempoRestante--);
          } else {
            _timer?.cancel();
            setState(() {});
          }
        });
      }
    }
    setState(() {});
  }

  void resetToLast() {
    if (ultimoTiempoSeteado <= 0) return;

    _timer?.cancel();
    if (ultimoTiempoSeteado == 14) {
      enviarComandoByte(253, 14); // 253 = T14
    } else if (ultimoTiempoSeteado == 24) {
      enviarComandoByte(254, 24); // 254 = H24
    } else {
      // Si fue manual, enviamos el mismo número como Byte
      enviarComandoByte(ultimoTiempoSeteado, ultimoTiempoSeteado);
    }
  }

  void setZero() {
    _timer?.cancel();
    enviarComandoByte(252, 0); // 252 = ZERO
    setState(() {
      tiempoRestante = 0;
      ultimoTiempoSeteado = 0;
    });
  }

  @override
  Widget build(BuildContext context) {
    bool isRunning = _timer != null && _timer!.isActive;

    return Scaffold(
      backgroundColor: azulUTMACH,
      appBar: AppBar(
        title: const Text("Contador 24 y 14 segundos", style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18)),
        centerTitle: true,
        actions: [
          IconButton(icon: const Icon(Icons.bluetooth, size: 30), onPressed: conectarBluetooth)
        ],
      ),
      body: SingleChildScrollView(
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 10.0),
          child: Column(
            children: [
              const SizedBox(height: 10),
              // Marcador Visual
              Container(
                width: 220, height: 220,
                decoration: BoxDecoration(
                  color: Colors.black,
                  borderRadius: BorderRadius.circular(25),
                  boxShadow: const [BoxShadow(color: Colors.black45, blurRadius: 15)],
                  border: Border.all(color: Colors.white, width: 5),
                ),
                alignment: Alignment.center,
                child: Text(
                  tiempoRestante.toString().padLeft(2, '0'),
                  style: const TextStyle(color: Colors.red, fontSize: 140, fontWeight: FontWeight.bold, fontFamily: 'monospace'),
                ),
              ),
              const SizedBox(height: 15),
              Text(logStatus, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 16, color: Color(0xFF1A237E))),
              const SizedBox(height: 25),

              // Botones Rápidos 14/24
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  _buildBotonTexturizado("14", () { 
                    _timer?.cancel(); 
                    ultimoTiempoSeteado = 14;
                    enviarComandoByte(253, 14); // Enviamos código 253
                  }),
                  _buildBotonTexturizado("24", () { 
                    _timer?.cancel(); 
                    ultimoTiempoSeteado = 24;
                    enviarComandoByte(254, 24); // Enviamos código 254
                  }),
                ],
              ),
              const SizedBox(height: 30),

              // Input Manual Blindado
              TextField(
                controller: _timeController,
                keyboardType: TextInputType.number,
                enableInteractiveSelection: false, 
                inputFormatters: [
                  FilteringTextInputFormatter.digitsOnly,
                  LengthLimitingTextInputFormatter(2), 
                ],
                style: const TextStyle(fontSize: 22, fontWeight: FontWeight.bold),
                decoration: InputDecoration(
                  hintText: "00-99 seg",
                  filled: true, fillColor: Colors.white,
                  prefixIcon: const Icon(Icons.timer, color: Colors.indigo),
                  suffixIcon: IconButton(
                    icon: const Icon(Icons.check_circle, color: Colors.green, size: 45),
                    onPressed: () {
                      int? val = int.tryParse(_timeController.text);
                      if (val != null && val >= 0 && val <= 99) {
                        _timer?.cancel();
                        ultimoTiempoSeteado = val;
                        enviarComandoByte(val, val); // Enviamos el número crudo
                      }
                      _timeController.clear();
                      FocusScope.of(context).unfocus();
                    },
                  ),
                  border: OutlineInputBorder(borderRadius: BorderRadius.circular(15)),
                ),
              ),

              const SizedBox(height: 40),

              // Controles Inferiores
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  _buildBotonControl(
                    child: const Text("0", style: TextStyle(fontSize: 35, fontWeight: FontWeight.bold, color: Colors.white)),
                    color: Colors.grey[850]!,
                    accion: setZero,
                    size: 75,
                  ),
                  _buildBotonControl(
                    child: Icon(isRunning ? Icons.pause : Icons.play_arrow, color: Colors.white, size: 65),
                    color: isRunning ? Colors.orange : Colors.green,
                    accion: togglePlayPause,
                    size: 110,
                  ),
                  _buildBotonControl(
                    child: const Icon(Icons.refresh, color: Colors.white, size: 45),
                    color: Colors.red,
                    accion: resetToLast,
                    size: 75,
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildBotonTexturizado(String t, VoidCallback onPress) {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: naranjaUTMACH,
        foregroundColor: Colors.black,
        minimumSize: const Size(120, 120),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
        elevation: 8,
      ),
      onPressed: onPress,
      child: Text(t, style: const TextStyle(fontSize: 45, fontWeight: FontWeight.bold)),
    );
  }

  Widget _buildBotonControl({required Widget child, required Color color, required VoidCallback accion, required double size}) {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: color,
        minimumSize: Size(size, size),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(25)),
        elevation: 10,
      ),
      onPressed: accion,
      child: child,
    );
  }
}