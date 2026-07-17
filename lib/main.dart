import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
import 'package:permission_handler/permission_handler.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  
  // BLOQUEO INTELIGENTE: Fija la app estrictamente en modo vertical
  SystemChrome.setPreferredOrientations([
    DeviceOrientation.portraitUp,
    DeviceOrientation.portraitDown,
  ]).then((_) {
    runApp(const UTMACHTimerApp());
  });
}

class UTMACHTimerApp extends StatelessWidget {
  const UTMACHTimerApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Contador 24 y 14 segundos',
      theme: ThemeData(primarySwatch: Colors.indigo),
      home: const TimerPage(),
    );
  }
}

class TimerPage extends StatefulWidget {
  const TimerPage({super.key});

  @override
  _TimerPageState createState() => _TimerPageState();
}

class _TimerPageState extends State<TimerPage> with WidgetsBindingObserver {
  BluetoothConnection? connection;
  int tiempoRestante = 0;
  int ultimoTiempoSeteado = 0;
  Timer? _timer;
  DateTime? _pausedTime;

  String logStatus = "Modo sin conexión Bluetooth";
  final TextEditingController _timeController = TextEditingController();

  final Color naranjaUTMACH = const Color(0xFFFF8C29);

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _timer?.cancel();
    connection?.dispose();
    connection = null;
    _timeController.dispose();
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.paused) {
      if (_timer != null && _timer!.isActive) {
        _pausedTime = DateTime.now();
      }
    } else if (state == AppLifecycleState.resumed) {
      if (_pausedTime != null && _timer != null && _timer!.isActive) {
        final elapsed = DateTime.now().difference(_pausedTime!).inSeconds;
        setState(() {
          tiempoRestante -= elapsed;
          if (tiempoRestante < 0) tiempoRestante = 0;
        });
        _pausedTime = null;
      }
    }
  }

  // --- LÓGICA BLUETOOTH ULTRA-RÁPIDA ---
  void conectarBluetooth() async {
    Map<Permission, PermissionStatus> statuses = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.locationWhenInUse,
    ].request();

    bool scanOk = statuses[Permission.bluetoothScan]?.isGranted ?? false;
    bool connectOk = statuses[Permission.bluetoothConnect]?.isGranted ?? false;

    if (!mounted) return;

    if (!scanOk || !connectOk) {
      bool permanentlyDenied =
          (statuses[Permission.bluetoothScan]?.isPermanentlyDenied ?? false) ||
          (statuses[Permission.bluetoothConnect]?.isPermanentlyDenied ?? false);

      setState(() => logStatus = "Permisos Bluetooth denegados");

      if (permanentlyDenied) {
        await showDialog(
          context: context,
          builder: (context) => AlertDialog(
            title: const Text("Permisos requeridos"),
            content: const Text("Activa los permisos de Bluetooth y Ubicación en Ajustes para conectar el marcador."),
            actions: [
              TextButton(onPressed: () => Navigator.pop(context), child: const Text("Cancelar")),
              TextButton(
                onPressed: () {
                  Navigator.pop(context);
                  openAppSettings();
                },
                child: const Text("Abrir Ajustes"),
              ),
            ],
          ),
        );
      }
      return;
    }

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
        if (!mounted) return;
        setState(() => logStatus = "Conectado a ${selectedDevice.name}");
      }
    } catch (e) {
      if (!mounted) return;
      setState(() => logStatus = "Error de conexión");
    }
  }

  // LA MAGIA DE ARTEMISA ESTÁ AQUÍ: Enviamos un Byte puro
  void enviarComandoByte(int byteCmd, int seg) async {
    if (connection != null && connection!.isConnected) {
      try {
        connection!.output.add(Uint8List.fromList([byteCmd]));
        await connection!.output.allSent;

        if (!mounted) return;
        setState(() {
          tiempoRestante = seg;
          logStatus = "Señal enviada: $byteCmd";
        });
      } catch (e) {
        if (!mounted) return;
        setState(() {
          tiempoRestante = seg;
          logStatus = "Bluetooth desconectado";
          connection = null;
        });
      }
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
          if (!mounted) {
            timer.cancel();
            return;
          }
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
    bool isConnected = connection != null && connection!.isConnected;
    
    // Heurística: Visibilidad del estado del sistema (Colores para conexión)
    Color btColor = isConnected ? Colors.greenAccent : Colors.redAccent;
    IconData btIcon = isConnected ? Icons.bluetooth_connected : Icons.bluetooth_disabled;

    return Scaffold(
      backgroundColor: Colors.black, // Color base de seguridad
      
      // La AppBar se vuelve transparente para que el fondo suba hasta arriba
      appBar: AppBar(
        title: const Text("CONTADOR 24 14 SEGUNDOS", style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18, color: Colors.white, letterSpacing: 1.2)),
        centerTitle: true,
        backgroundColor: Colors.transparent, // Barra transparente
        elevation: 0, // Sin sombra
        actions: [
          IconButton(
            icon: Icon(btIcon, size: 30, color: btColor),
            tooltip: isConnected ? "Bluetooth Conectado" : "Conectar Bluetooth", 
            onPressed: conectarBluetooth
          )
        ],
      ),
      extendBodyBehindAppBar: true, // Esto hace que el fondo ocupe TODA la pantalla

      // EL CONTENEDOR INTELIGENTE PARA EL FONDO
      body: Container(
        width: double.infinity,
        height: double.infinity,
        decoration: const BoxDecoration(
          image: DecorationImage(
            image: AssetImage('assets/images/fondo.png'), // Tu imagen vertical
            fit: BoxFit.cover, // El recorte inteligente que evita que se achate
          ),
        ),
        
        // SafeArea protege que el contenido no quede debajo de la muesca de la cámara
        child: SafeArea(
          child: SingleChildScrollView(
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 10.0),
              child: Column(
                children: [
                  const SizedBox(height: 10),
                  
                  // Marcador Visual (Responsive seguro y con márgenes para que no se vea asfixiado)
                  Container(
                    width: 260, height: 260,
                    decoration: BoxDecoration(
                      color: Colors.black.withOpacity(0.85), // Translúcido
                      borderRadius: BorderRadius.circular(25),
                      boxShadow: const [BoxShadow(color: Colors.black45, blurRadius: 15)],
                      border: Border.all(color: Colors.white, width: 4),
                    ),
                    alignment: Alignment.center,
                    child: Padding(
                      padding: const EdgeInsets.all(35.0), // Margen exterior real
                      child: FittedBox(
                        fit: BoxFit.contain,
                        child: Text(
                          tiempoRestante.toString().padLeft(2, '0'),
                          style: const TextStyle(color: Colors.red, fontSize: 160, fontWeight: FontWeight.bold, fontFamily: 'monospace'),
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(height: 15),
                  
                  // Fondo semi-transparente para el texto de log dinámico (Feedback de status)
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                    decoration: BoxDecoration(
                      color: isConnected ? Colors.green.withOpacity(0.85) : Colors.redAccent.withOpacity(0.85),
                      borderRadius: BorderRadius.circular(20),
                    ),
                    child: Text(logStatus, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 14, color: Colors.white)),
                  ),
                  const SizedBox(height: 25),

                  // Botones Rápidos 14/24
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                    children: [
                      _buildBotonTexturizado("14", () {
                        _timer?.cancel();
                        ultimoTiempoSeteado = 14;
                        enviarComandoByte(253, 14); 
                      }),
                      _buildBotonTexturizado("24", () {
                        _timer?.cancel();
                        ultimoTiempoSeteado = 24;
                        enviarComandoByte(254, 24); 
                      }),
                    ],
                  ),
                  const SizedBox(height: 30),

                  // Input Manual Blindado (con fondo translúcido blanco)
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
                      hintText: "Manual: 00-99",
                      filled: true, fillColor: Colors.white.withOpacity(0.95), 
                      prefixIcon: const Icon(Icons.timer, color: Colors.indigo),
                      suffixIcon: IconButton(
                        icon: const Icon(Icons.check_circle, color: Colors.green, size: 45),
                        onPressed: () {
                          int? val = int.tryParse(_timeController.text);
                          if (val != null && val >= 0 && val <= 99) {
                            _timer?.cancel();
                            if (val == 0) {
                              setZero();
                            } else {
                              ultimoTiempoSeteado = val;
                              enviarComandoByte(val, val); 
                            }
                          }
                          _timeController.clear();
                          FocusScope.of(context).unfocus();
                        },
                      ),
                      border: OutlineInputBorder(borderRadius: BorderRadius.circular(15)),
                    ),
                  ),

                  const SizedBox(height: 40),

                  // Controles Inferiores Play/Pause/Reset
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                    children: [
                      _buildBotonControl(
                        child: const Text("0", style: TextStyle(fontSize: 35, fontWeight: FontWeight.bold, color: Colors.white)),
                        color: Colors.grey[850]!.withOpacity(0.95),
                        accion: setZero,
                        size: 75,
                      ),
                      _buildBotonControl(
                        child: Icon(isRunning ? Icons.pause : Icons.play_arrow, color: Colors.white, size: 65),
                        color: isRunning ? Colors.orange.withOpacity(0.95) : Colors.green.withOpacity(0.95),
                        accion: togglePlayPause,
                        size: 110,
                      ),
                      _buildBotonControl(
                        child: const Icon(Icons.refresh, color: Colors.white, size: 45),
                        color: Colors.red.withOpacity(0.95),
                        accion: resetToLast,
                        size: 75,
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildBotonTexturizado(String t, VoidCallback onPress) {
    return ElevatedButton(
      style: ElevatedButton.styleFrom(
        backgroundColor: naranjaUTMACH.withOpacity(0.95),
        foregroundColor: Colors.black,
        minimumSize: const Size(120, 120),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(20),
          side: const BorderSide(color: Colors.white, width: 2),
        ),
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
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(25),
          side: const BorderSide(color: Colors.white24, width: 1)
        ),
        elevation: 10,
      ),
      onPressed: accion,
      child: child,
    );
  }
}