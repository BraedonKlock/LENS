import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  RefreshControl,
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { useNotifications } from '../context/NotificationContext';

const ACCENT = '#F5C518';

function relativeTime(date) {
  const d = date instanceof Date ? date : new Date(date);
  const mins = Math.floor((Date.now() - d) / 60000);
  if (mins < 1) return 'Just now';
  if (mins < 60) return `${mins}m ago`;
  const hrs = Math.floor(mins / 60);
  if (hrs < 24) return `${hrs}h ago`;
  return `${Math.floor(hrs / 24)}d ago`;
}

function IncidentCard({ incident, onPress }) {
  return (
    <TouchableOpacity
      style={[styles.card, !incident.read && styles.cardUnread]}
      onPress={onPress}
      activeOpacity={0.75}
    >
      <View style={[styles.strip, !incident.read && styles.stripUnread]} />
      <View style={styles.cardBody}>
        <View style={styles.cardTop}>
          <View style={styles.titleRow}>
            {!incident.read && <View style={styles.unreadDot} />}
            <Text style={styles.cardTitle} numberOfLines={1}>
              {incident.title}
            </Text>
          </View>
        </View>

        <View style={styles.metaRow}>
          <Ionicons name="videocam" size={13} color="#8B949E" />
          <Text style={styles.metaText} numberOfLines={1}>
            {incident.camera}
          </Text>
        </View>

        <View style={styles.cardFooter}>
          <View style={styles.metaRow}>
            <Ionicons name="time-outline" size={13} color="#8B949E" />
            <Text style={styles.metaText}>{relativeTime(incident.timestamp)}</Text>
          </View>
          <View style={styles.viewClip}>
            <Ionicons name="play-circle" size={15} color="#58A6FF" />
            <Text style={styles.viewClipText}>View Clip</Text>
          </View>
        </View>
      </View>
    </TouchableOpacity>
  );
}

export default function IncidentsScreen({ navigation }) {
  const { incidents, unreadCount, markAllAsRead, refresh, loading } = useNotifications();
  const insets = useSafeAreaInsets();

  return (
    <View style={[styles.container, { paddingTop: insets.top }]}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Incidents</Text>
        {unreadCount > 0 && (
          <TouchableOpacity onPress={markAllAsRead} style={styles.markAllBtn}>
            <Text style={styles.markAllText}>Mark all read</Text>
          </TouchableOpacity>
        )}
      </View>

      {incidents.length === 0 ? (
        <View style={styles.empty}>
          <Ionicons name="shield-checkmark-outline" size={72} color="#21262D" />
          <Text style={styles.emptyTitle}>All Clear</Text>
          <Text style={styles.emptySubtitle}>No security incidents detected</Text>
        </View>
      ) : (
        <FlatList
          data={incidents}
          keyExtractor={item => item.id}
          renderItem={({ item }) => (
            <IncidentCard
              incident={item}
              onPress={() => navigation.navigate('IncidentDetail', { incident: item })}
            />
          )}
          contentContainerStyle={[styles.list, { paddingBottom: insets.bottom + 24 }]}
          showsVerticalScrollIndicator={false}
          refreshControl={<RefreshControl refreshing={loading} onRefresh={refresh} tintColor="#58A6FF" />}
        />
      )}
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
    paddingHorizontal: 16,
    paddingVertical: 16,
    borderBottomWidth: 1,
    borderBottomColor: '#21262D',
  },
  headerTitle: {
    color: '#E6EDF3',
    fontSize: 24,
    fontWeight: 'bold',
  },
  markAllBtn: {
    paddingVertical: 6,
    paddingHorizontal: 10,
    borderRadius: 8,
    backgroundColor: '#161B22',
    borderWidth: 1,
    borderColor: '#21262D',
  },
  markAllText: {
    color: ACCENT,
    fontSize: 13,
    fontWeight: '600',
  },
  list: {
    padding: 16,
  },
  card: {
    backgroundColor: '#161B22',
    borderRadius: 14,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#21262D',
    flexDirection: 'row',
    overflow: 'hidden',
  },
  strip: {
    width: 4,
    backgroundColor: ACCENT,
  },
  stripUnread: {
    backgroundColor: '#FF4444',
  },
  cardUnread: {
    borderColor: '#FF444433',
    backgroundColor: '#1A0A0A',
  },
  unreadDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: '#FF4444',
    flexShrink: 0,
  },
  cardBody: {
    flex: 1,
    padding: 14,
    gap: 8,
  },
  cardTop: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    gap: 8,
  },
  titleRow: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
    gap: 8,
  },
  cardTitle: {
    color: '#E6EDF3',
    fontSize: 16,
    fontWeight: '600',
    flex: 1,
  },
  metaRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 5,
  },
  metaText: {
    color: '#8B949E',
    fontSize: 13,
    flex: 1,
  },
  cardFooter: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  viewClip: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  viewClipText: {
    color: '#58A6FF',
    fontSize: 13,
    fontWeight: '600',
  },
  empty: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 14,
  },
  emptyTitle: {
    color: '#E6EDF3',
    fontSize: 22,
    fontWeight: 'bold',
  },
  emptySubtitle: {
    color: '#8B949E',
    fontSize: 15,
  },
});
