import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  ActivityIndicator,
} from 'react-native';
import { useVideoPlayer, VideoView } from 'expo-video';
import { Ionicons } from '@expo/vector-icons';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { useNotifications } from '../context/NotificationContext';
import { authFetch } from '../utils/api';

function formatTimestamp(iso) {
  return new Date(iso).toLocaleString('en-US', {
    year:   'numeric',
    month:  'short',
    day:    'numeric',
    hour:   '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
}

export default function IncidentDetailScreen({ route, navigation }) {
  const { incident }    = route.params;
  const { markAsRead }  = useNotifications();
  const insets          = useSafeAreaInsets();

  const [clipUrl, setClipUrl] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError]     = useState(false);

  const player = useVideoPlayer(clipUrl, p => {
    p.loop = false;
  });

  useEffect(() => {
    markAsRead(incident.id);
    fetchClipUrl();
  }, []);

  async function fetchClipUrl() {
    try {
      const res = await authFetch(`/incidents/${incident.id}/clip`);
      if (!res.ok) { setError(true); setLoading(false); return; }
      const data = await res.json();
      setClipUrl(data.url);
      setLoading(false);
    } catch {
      setError(true);
      setLoading(false);
    }
  }

  return (
    <View style={[styles.container, { paddingTop: insets.top }]}>
      {/* Header */}
      <View style={styles.header}>
        <TouchableOpacity onPress={() => navigation.goBack()} style={styles.backBtn}>
          <Ionicons name="chevron-back" size={26} color="#E6EDF3" />
        </TouchableOpacity>
        <Text style={styles.headerTitle} numberOfLines={1}>Incident Detail</Text>
        <View style={{ width: 44 }} />
      </View>

      <ScrollView showsVerticalScrollIndicator={false} contentContainerStyle={{ paddingBottom: insets.bottom + 32 }}>
        {/* Video player */}
        <View style={styles.videoWrapper}>
          {loading && !error && (
            <View style={styles.videoOverlay}>
              <ActivityIndicator size="large" color="#58A6FF" />
              <Text style={styles.loadingText}>Loading clip…</Text>
            </View>
          )}
          {error && (
            <View style={styles.videoOverlay}>
              <Ionicons name="warning-outline" size={40} color="#FF4444" />
              <Text style={styles.errorText}>Could not load video clip</Text>
            </View>
          )}
          {clipUrl && !error && (
            <VideoView
              player={player}
              style={styles.video}
              fullscreenOptions={{ supportedOrientations: ['landscape'] }}
              allowsPictureInPicture
            />
          )}
          <View style={styles.clipBadge}>
            <View style={styles.recDot} />
            <Text style={styles.clipBadgeText}>CLIP</Text>
          </View>
        </View>

        {/* Info */}
        <View style={styles.info}>
          <Text style={styles.incidentTitle}>{incident.title}</Text>
          <View style={styles.detailsCard}>
            <DetailRow icon="videocam"           label="Camera"    value={incident.camera} />
            <DetailRow icon="time"               label="Detected"  value={formatTimestamp(incident.timestamp)} />
            <DetailRow icon="document-text-outline" label="Description" value={incident.description} />
            <DetailRow icon="checkmark-circle"   label="Review Status" value="Reviewed" valueColor="#2ECC71" noBorder />
          </View>
        </View>
      </ScrollView>
    </View>
  );
}

function DetailRow({ icon, label, value, valueColor, noBorder }) {
  return (
    <View style={[styles.detailRow, noBorder && { borderBottomWidth: 0 }]}>
      <Ionicons name={icon} size={17} color="#8B949E" style={{ marginTop: 2 }} />
      <View style={styles.detailContent}>
        <Text style={styles.detailLabel}>{label}</Text>
        <Text style={[styles.detailValue, valueColor && { color: valueColor }]}>{value}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0D1117',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingHorizontal: 8,
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#21262D',
  },
  backBtn: {
    width: 44,
    height: 44,
    alignItems: 'center',
    justifyContent: 'center',
  },
  headerTitle: {
    color: '#E6EDF3',
    fontSize: 18,
    fontWeight: 'bold',
    flex: 1,
    textAlign: 'center',
  },
  videoWrapper: {
    backgroundColor: '#000',
    aspectRatio: 16 / 9,
    position: 'relative',
  },
  video: {
    flex: 1,
  },
  videoOverlay: {
    ...StyleSheet.absoluteFillObject,
    backgroundColor: '#161B22',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 12,
    zIndex: 1,
  },
  loadingText: {
    color: '#8B949E',
    fontSize: 14,
  },
  errorText: {
    color: '#FF4444',
    fontSize: 14,
    marginTop: 4,
  },
  clipBadge: {
    position: 'absolute',
    top: 10,
    left: 10,
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: 'rgba(0,0,0,0.6)',
    borderRadius: 6,
    paddingHorizontal: 8,
    paddingVertical: 4,
    gap: 5,
  },
  recDot: {
    width: 7,
    height: 7,
    borderRadius: 4,
    backgroundColor: '#FF4444',
  },
  clipBadgeText: {
    color: 'white',
    fontSize: 11,
    fontWeight: 'bold',
    letterSpacing: 1,
  },
  info: {
    padding: 16,
    gap: 12,
  },
  incidentTitle: {
    color: '#E6EDF3',
    fontSize: 24,
    fontWeight: 'bold',
  },
  detailsCard: {
    backgroundColor: '#161B22',
    borderRadius: 14,
    borderWidth: 1,
    borderColor: '#21262D',
    overflow: 'hidden',
    marginTop: 4,
  },
  detailRow: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    padding: 14,
    gap: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#21262D',
  },
  detailContent: {
    flex: 1,
    gap: 4,
  },
  detailLabel: {
    color: '#8B949E',
    fontSize: 11,
    fontWeight: '700',
    textTransform: 'uppercase',
    letterSpacing: 0.8,
  },
  detailValue: {
    color: '#E6EDF3',
    fontSize: 15,
    lineHeight: 22,
  },
});
