import React, { useState, useEffect, useRef } from 'react';
import {
  Zap,
  Battery,
  Activity,
  Cpu,
  LayoutDashboard,
  Terminal,
  RefreshCcw,
  Info,
  Database,
  User as UserIcon,
  LogOut,
  X
} from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { ref, onValue } from 'firebase/database';
import {
  signInAnonymously,
  onAuthStateChanged,
  signInWithEmailAndPassword,
  signOut
} from 'firebase/auth';
import type { User } from 'firebase/auth';
import { db, auth } from './firebase';
import './App.css';

interface DashboardData {
  batteryPower: number;
  batteryCurrent: number;
  gridCurrent: number;
  soc: number;
  statusMsg: string;
  logs: string;
  version: string;
  connected: boolean;
  authenticated: boolean;
  relayIdx: number;
  lastUpdate: number;
}

const App: React.FC = () => {
  const [data, setData] = useState<DashboardData>({
    batteryPower: 0,
    batteryCurrent: 0,
    gridCurrent: 0,
    soc: 0,
    statusMsg: 'Initializace...',
    logs: 'Connecting to Firebase...',
    version: '1.0.0-rev',
    connected: false,
    authenticated: false,
    relayIdx: 0,
    lastUpdate: 0
  });

  const [isAdmin, setIsAdmin] = useState(false);
  const [showLogin, setShowLogin] = useState(false);
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [authError, setAuthError] = useState('');
  const [activeTab, setActiveTab] = useState<'dashboard' | 'console'>('dashboard');

  // Force re-render every 30s to update "Online/Offline" status
  const [, setTick] = useState(0);
  useEffect(() => {
    const interval = setInterval(() => setTick(t => t + 1), 30000);
    return () => clearInterval(interval);
  }, []);

  const consoleRef = useRef<HTMLPreElement>(null);

  // Auto-scroll console
  useEffect(() => {
    if (consoleRef.current) {
      consoleRef.current.scrollTop = consoleRef.current.scrollHeight;
    }
  }, [data.logs]);

  // Check if system is online (last update < 5 minutes)
  const isSystemOnline = data.lastUpdate > 0 && (Math.floor(Date.now() / 1000) - data.lastUpdate) < 300;

  // Handle Authentication and Firebase connection
  useEffect(() => {
    let unsubscribeEnergy: () => void;

    const unsubscribeAuth = onAuthStateChanged(auth, (user: User | null) => {
      if (user) {
        setData(prev => ({ ...prev, authenticated: true }));
        setIsAdmin(!user.isAnonymous);

        // If not already subscribed, subscribe now
        if (!unsubscribeEnergy) {
          const energyRef = ref(db, 'energy_data');
          unsubscribeEnergy = onValue(energyRef, (snapshot: any) => {
            if (snapshot.exists()) {
              const val = snapshot.val();
              setData(prev => ({
                ...prev,
                batteryPower: val.battery_P || 0,
                batteryCurrent: val.battery_I || 0,
                gridCurrent: val.grid_I || 0,
                soc: val.battery_soc || 0,
                statusMsg: val.status_msg || prev.statusMsg,
                version: val.version || prev.version,
                logs: val.console_log || prev.logs,
                relayIdx: val.relay_idx || 0,
                connected: true,
                lastUpdate: val.last_update || 0
              }));
            } else {
              setData(prev => ({
                ...prev,
                statusMsg: 'Čekám na data z procesoru...',
                connected: true
              }));
            }
          });
        }
      } else {
        // Fallback to anonymous for basic viewing
        setActiveTab('dashboard');
        signInAnonymously(auth).catch(err => {
          console.error("Anon login error:", err);
          setData(prev => ({ ...prev, statusMsg: 'Chyba připojení k serveru' }));
        });
      }
    });

    return () => {
      unsubscribeAuth();
      if (unsubscribeEnergy) unsubscribeEnergy();
    };
  }, []);

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    setAuthError('');
    try {
      await signInWithEmailAndPassword(auth, email, password);
      setShowLogin(false);
      setEmail('');
      setPassword('');
    } catch (err: any) {
      setAuthError('Neplatné jméno nebo heslo');
    }
  };

  const handleLogout = () => {
    signOut(auth);
  };

  return (
    <div className={`app-wrapper ${isAdmin ? 'has-sidebar' : ''}`}>
      {isAdmin && (
        <aside className="sidebar">
          <div className="sidebar-logo">
            <Zap size={24} color="#38bdf8" />
            <span>TUCAPY</span>
          </div>
          <nav className="sidebar-nav">
            <button 
              className={`nav-item ${activeTab === 'dashboard' ? 'active' : ''}`}
              onClick={() => setActiveTab('dashboard')}
            >
              <LayoutDashboard size={20} />
              <span>Monitoring</span>
            </button>
            <button 
              className={`nav-item ${activeTab === 'console' ? 'active' : ''}`}
              onClick={() => setActiveTab('console')}
            >
              <Terminal size={20} />
              <span>Systémová konzole</span>
            </button>
          </nav>
          <div className="sidebar-footer">
            <button className="logout-btn" onClick={handleLogout}>
              <LogOut size={18} />
              <span>Odhlásit</span>
            </button>
          </div>
        </aside>
      )}

      <div className="main-content">
        <header>
          <motion.div
            initial={{ opacity: 0, x: -20 }}
            animate={{ opacity: 1, x: 0 }}
            className="header-left"
          >
            {isAdmin ? (
              <h2>{activeTab === 'dashboard' ? 'Energetický přehled' : 'Systémové záznamy'}</h2>
            ) : (
              <>
                <h1>Tucapy Energy</h1>
                <div className="version-tag">{data.version}</div>
              </>
            )}
          </motion.div>

          <motion.div
            initial={{ opacity: 0, scale: 0.8 }}
            animate={{ opacity: 1, scale: 1 }}
            className="header-right"
            style={{ display: 'flex', gap: '8px' }}
          >
            {!isAdmin && (
              <button className="icon-button login" onClick={() => setShowLogin(true)} title="Admin vstup">
                <UserIcon size={16} />
              </button>
            )}

            {data.authenticated && (
              <div className={`status-badge ${isSystemOnline ? 'secure' : 'offline'}`}>
                <Cpu size={12} />
                {isSystemOnline ? 'ESP32 Online' : 'ESP32 Offline'}
              </div>
            )}
            <div className={`status-badge ${data.connected ? '' : 'offline'}`}>
              <Database size={14} />
              {data.connected ? 'Cloud OK' : 'Connecting...'}
            </div>
          </motion.div>
        </header>

        <AnimatePresence mode="wait">
          {activeTab === 'dashboard' ? (
            <motion.div
              key="dashboard"
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              className="view-container"
            >
              <div className="grid">
                <StatCard
                  icon={<Zap size={20} color="#38bdf8" />}
                  label="Battery Power"
                  value={data.batteryPower}
                  unit="kW"
                  index={0}
                />
                <StatCard
                  icon={<Activity size={20} color="#10b981" />}
                  label="Battery Current"
                  value={data.batteryCurrent}
                  unit="A"
                  index={1}
                />
                <StatCard
                  icon={<Cpu size={20} color="#fbbf24" />}
                  label="Grid Current"
                  value={data.gridCurrent}
                  unit="A"
                  index={2}
                />
                <StatCard
                  icon={<Battery size={20} color="#38bdf8" />}
                  label="Battery SOC"
                  value={data.soc}
                  unit="%"
                  showBar
                  index={3}
                />
              </div>

              <motion.div
                initial={{ opacity: 0, y: 20 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: 0.3 }}
                className="relays-container"
              >
                <div className="relays-header">
                  <Activity size={16} />
                  <span>ACTIVE OUTPUTS (RELAYS)</span>
                </div>
                <div className="relays-grid">
                  {[...Array(8)].map((_, i) => (
                    <div key={i} className={`relay-item ${i < data.relayIdx ? 'active' : ''}`}>
                      <div className="relay-led" />
                      <span className="relay-label">OUT{i + 1}</span>
                    </div>
                  ))}
                </div>
              </motion.div>

              <motion.div
                initial={{ opacity: 0, y: 20 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ delay: 0.4 }}
                className="status-message"
              >
                <div style={{ display: 'flex', alignItems: 'center', gap: '8px', justifyContent: 'center' }}>
                  <Info size={16} />
                  {data.statusMsg}
                </div>
              </motion.div>
            </motion.div>
          ) : (
            <motion.div
              key="console"
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -10 }}
              className="view-container console-view"
            >
              <div className="console-card full-page">
                <div className="console-header">
                  <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
                    <Terminal size={14} />
                    <span>SYSTEM CONSOLE LOGS FROM ESP32</span>
                  </div>
                  <motion.div
                    animate={{ rotate: 360 }}
                    transition={{ duration: 4, repeat: Infinity, ease: "linear" }}
                  >
                    <RefreshCcw size={12} color="#94a3b8" />
                  </motion.div>
                </div>
                <pre ref={consoleRef} className="console-body">
                  {data.logs}
                </pre>
              </div>
            </motion.div>
          )}
        </AnimatePresence>

        <footer style={{ marginTop: 'auto', paddingTop: '40px' }}>
          &copy; 2024 Tucapy Energy Regulation &bull; {data.version}
        </footer>
      </div>

      <AnimatePresence>
        {showLogin && (
          <motion.div 
            className="modal-overlay"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
          >
            <motion.div 
              className="login-modal"
              initial={{ scale: 0.9, y: 20 }}
              animate={{ scale: 1, y: 0 }}
              exit={{ scale: 0.9, y: 20 }}
            >
              <div className="modal-header">
                <h3>Admin Login</h3>
                <button className="close-btn" onClick={() => setShowLogin(false)}><X size={18} /></button>
              </div>
              <form onSubmit={handleLogin}>
                <div className="input-group">
                  <label>E-mail</label>
                  <input 
                    type="email" 
                    value={email} 
                    onChange={(e) => setEmail(e.target.value)} 
                    placeholder="admin@example.com"
                    required 
                  />
                </div>
                <div className="input-group">
                  <label>Heslo</label>
                  <input 
                    type="password" 
                    value={password} 
                    onChange={(e) => setPassword(e.target.value)} 
                    placeholder="••••••••"
                    required 
                  />
                </div>
                {authError && <div className="auth-error-msg">{authError}</div>}
                <button type="submit" className="login-submit-btn">Přihlásit se</button>
              </form>
            </motion.div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
};

interface StatCardProps {
  icon: React.ReactNode;
  label: string;
  value: number;
  unit: string;
  showBar?: boolean;
  index: number;
}

const StatCard: React.FC<StatCardProps> = ({ icon, label, value, unit, showBar, index }) => {
  return (
    <motion.div
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ delay: 0.1 * index }}
      className="card"
    >
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
        <div className="card-title">{label}</div>
        {icon}
      </div>
      <div className="card-value-container">
        <AnimatePresence mode="wait">
          <motion.span
            key={value}
            initial={{ opacity: 0, y: 10 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -10 }}
            transition={{ duration: 0.2 }}
            className="card-value"
          >
            {value}
          </motion.span>
        </AnimatePresence>
        <span className="card-unit">{unit}</span>
      </div>
      {showBar && (
        <div className="soc-bar-container">
          <motion.div
            className="soc-bar"
            initial={{ width: 0 }}
            animate={{ width: `${value}%` }}
            transition={{ type: "spring", stiffness: 50 }}
            style={{
              backgroundColor: value < 20 ? '#ef4444' : (value < 50 ? '#fbbf24' : '#10b981')
            }}
          />
        </div>
      )}
    </motion.div>
  );
}

export default App;

