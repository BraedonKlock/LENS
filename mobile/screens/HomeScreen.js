import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Platform,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import * as Notifications from 'expo-notifications';
import { useNotifications } from '../context/NotificationContext';

const CAMERAS = [
  { id: '1', name: 'Front Entrance', location: 'Camera 1', status: 'online' },
  { id: '2', name: 'Side Exit', location: 'Camera 2', status: 'online' },
  { id: '3', name: 'Parking Lot', location: 'Camera 3', status: 'online' },
  { id: '4', name: 'Server Room', location: 'Camera 4', status: 'offline' },
];

const ALERT_TYPES = [
  { title: 'Person Detected', subtitle: 'Front Entrance — Camera 1', severity: 'high', video: 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerBlazes.mp4' },
  { title: 'Motion Alert', subtitle: 'Parking Lot — Camera 3', severity: 'medium', video: 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerEscapes.mp4' },
  { title: 'Unauthorized Access', subtitle: 'Server Room — Camera 4', severity: 'high', video: 'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerJoyrides.mp4' },
];

async function sendTestAlert() {
  const alert = ALERT_TYPES[Math.floor(Math.random() * ALERT_TYPES.length)];
  await Notifications.scheduleNotificationAsync({
    content: {
      title: `⚠️ ${alert.title}`,
      subtitle: alert.subtitle,
      body: `LENS AI flagged activity on ${alert.subtitle}. Tap to view the incident clip.`,
      data: { severity: alert.severity, videoUrl: alert.video },
      sound: true,
    },
    trigger: null,
  });
}

export default function HomeScreen({ navigation }) {
  const { incidents, unreadCount } = useNotifications();
  const insets = useSafeAreaInsets();
  const onlineCount = CAMERAS.filter(c => c.status === 'online').length;

  return (
    <ScrollView
      style={styles.container}
      contentContainerStyle={[styles.content, { paddingBottom: insets.bottom + 32 }]}
      showsVerticalScrollIndicator={false}
    >
      {/* Alert banner */}
      {unreadCount > 0 && (
        <TouchableOpacity
          style={styles.alertBanner}
          onPress={() => navigation.navigate('Incidents')}
          activeOpacity={0.75}
        >
          <Ionicons name="alert-circle" size={20} color="#FF4444" />
          <Text style={styles.alertBannerText}>
            {unreadCount} new alert{unreadCount !== 1 ? 's' : ''} — tap to review
          </Text>
          <Ionicons name="chevron-forward" size={16} color="#FF4444" />
        </TouchableOpacity>
      )}

      {/* Stat cards */}
      <View style={styles.statsRow}>
        <StatCard
          icon="videocam"
          iconColor="#2ECC71"
          borderColor="#2ECC7133"
          value={onlineCount}
          label="Online"
        />
        <StatCard
          icon="warning"
          iconColor="#FF4444"
          borderColor="#FF444433"
          value={unreadCount}
          label="Unread"
        />
        <StatCard
          icon="shield-checkmark"
          iconColor="#58A6FF"
          borderColor="#58A6FF33"
          value={incidents.length}
          label="Total"
        />
      </View>

      {/* System status */}
      <Section title="System Status">
        <View style={styles.systemCard}>
          <SystemRow icon="pulse" label="LENS Engine" value="Running" valueColor="#2ECC71" />
          <SystemRow icon="eye" label="AI Detection" value="Active" valueColor="#2ECC71" />
          <SystemRow icon="server" label="Backend API" value="Connected" valueColor="#2ECC71" />
          <SystemRow icon="cloud-upload" label="Storage" value="74% used" valueColor="#FF9800" noBorder />
        </View>
      </Section>

      {/* Camera status */}
      <Section title="Cameras">
        {CAMERAS.map(cam => (
          <View key={cam.id} style={styles.cameraRow}>
            <View style={[styles.dot, { backgroundColor: cam.status === 'online' ? '#2ECC71' : '#FF4444' }]} />
            <Ionicons
              name={cam.status === 'online' ? 'videocam' : 'videocam-off'}
              size={18}
              color={cam.status === 'online' ? '#E6EDF3' : '#8B949E'}
              style={{ marginRight: 10 }}
            />
            <View style={{ flex: 1 }}>
              <Text style={[styles.cameraName, cam.status === 'offline' && { color: '#8B949E' }]}>
                {cam.name}
              </Text>
              <Text style={styles.cameraLocation}>{cam.location}</Text>
            </View>
            <Text style={[styles.cameraStatusText, { color: cam.status === 'online' ? '#2ECC71' : '#FF4444' }]}>
              {cam.status.toUpperCase()}
            </Text>
          </View>
        ))}
      </Section>

      {/* Test notification */}
      <Section title="Testing">
        <TouchableOpacity style={styles.testBtn} onPress={sendTestAlert} activeOpacity={0.8}>
          <Ionicons name="notifications" size={20} color="#0D1117" />
          <Text style={styles.testBtnText}>Send Test Alert</Text>
        </TouchableOpacity>
        <Text style={styles.hint}>Simulates an incoming LENS security alert — watch the bell icon react</Text>
      </Section>
    </ScrollView>
  );
}

function StatCard({ icon, iconColor, borderColor, value, label }) {
  return (
    <View style={[styles.statCard, { borderColor }]}>
      <Ionicons name={icon} size={26} color={iconColor} />
      <Text style={styles.statValue}>{value}</Text>
      <Text style={styles.statLabel}>{label}</Text>
    </View>
  );
}

function Section({ title, children }) {
  return (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>{title}</Text>
      {children}
    </View>
  );
}

function SystemRow({ icon, label, value, valueColor, noBorder }) {
  return (
    <View style={[styles.systemRow, noBorder && { borderBottomWidth: 0 }]}>
      <Ionicons name={icon} size={16} color="#8B949E" style={{ marginRight: 10 }} />
      <Text style={styles.systemLabel}>{label}</Text>
      <Text style={[styles.systemValue, { color: valueColor }]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0D1117',
  },
  content: {
    padding: 16,
  },
  alertBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1A0A0A',
    borderColor: '#FF4444',
    borderWidth: 1,
    borderRadius: 12,
    padding: 12,
    marginBottom: 16,
    gap: 8,
  },
  alertBannerText: {
    color: '#FF4444',
    flex: 1,
    fontWeight: '600',
    fontSize: 14,
  },
  statsRow: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 24,
  },
  statCard: {
    flex: 1,
    backgroundColor: '#161B22',
    borderRadius: 14,
    padding: 14,
    alignItems: 'center',
    gap: 6,
    borderWidth: 1,
  },
  statValue: {
    color: '#E6EDF3',
    fontSize: 26,
    fontWeight: 'bold',
  },
  statLabel: {
    color: '#8B949E',
    fontSize: 11,
    textAlign: 'center',
    letterSpacing: 0.3,
  },
  section: {
    marginBottom: 24,
  },
  sectionTitle: {
    color: '#8B949E',
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 1.2,
    textTransform: 'uppercase',
    marginBottom: 10,
  },
  systemCard: {
    backgroundColor: '#161B22',
    borderRadius: 14,
    borderWidth: 1,
    borderColor: '#21262D',
    overflow: 'hidden',
  },
  systemRow: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 14,
    borderBottomWidth: 1,
    borderBottomColor: '#21262D',
  },
  systemLabel: {
    color: '#E6EDF3',
    flex: 1,
    fontSize: 14,
  },
  systemValue: {
    fontSize: 13,
    fontWeight: '600',
  },
  cameraRow: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#161B22',
    borderRadius: 12,
    padding: 14,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#21262D',
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 10,
  },
  cameraName: {
    color: '#E6EDF3',
    fontSize: 15,
    fontWeight: '500',
  },
  cameraLocation: {
    color: '#8B949E',
    fontSize: 12,
    marginTop: 2,
  },
  cameraStatusText: {
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 0.5,
  },
  testBtn: {
    backgroundColor: '#58A6FF',
    borderRadius: 14,
    padding: 16,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 10,
  },
  testBtnText: {
    color: '#0D1117',
    fontWeight: 'bold',
    fontSize: 16,
  },
  hint: {
    color: '#8B949E',
    fontSize: 12,
    textAlign: 'center',
    marginTop: 10,
    lineHeight: 18,
  },
});
