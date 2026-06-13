import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import * as Notifications from 'expo-notifications';
import { useNotifications } from '../context/NotificationContext';

const AISLES = [
  'Aisle 1 — Electronics',
  'Aisle 3 — Clothing',
  'Aisle 5 — Cosmetics',
  'Aisle 7 — Sporting Goods',
  'Aisle 9 — Toys',
  'Aisle 12 — Grocery',
];

const VIDEOS = [
  'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerBlazes.mp4',
  'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerEscapes.mp4',
  'https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerJoyrides.mp4',
];

async function sendTestAlert() {
  const aisle = AISLES[Math.floor(Math.random() * AISLES.length)];
  const video = VIDEOS[Math.floor(Math.random() * VIDEOS.length)];
  await Notifications.scheduleNotificationAsync({
    content: {
      title: '⚠️ Concealment Detected',
      subtitle: aisle,
      body: `LENS AI flagged concealment activity in ${aisle}. Tap to view the incident clip.`,
      data: { severity: 'high', videoUrl: video },
      sound: true,
    },
    trigger: null,
  });
}

export default function HomeScreen({ navigation }) {
  const { incidents, unreadCount } = useNotifications();
  const insets = useSafeAreaInsets();

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
          label="Total Incidents"
        />
      </View>

      {/* Test notification */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Testing</Text>
        <TouchableOpacity style={styles.testBtn} onPress={sendTestAlert} activeOpacity={0.8}>
          <Ionicons name="notifications" size={20} color="#0D1117" />
          <Text style={styles.testBtnText}>Send Test Alert</Text>
        </TouchableOpacity>
        <Text style={styles.hint}>Simulates an incoming LENS security alert — watch the bell icon react</Text>
      </View>
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
